# Smart Surveillance Camera 만들기
Tizen의 Camera API, Vision API, Peripheral API, IoT.js, SmartThing 을 이용해서 Smart Surveillance Camera를 만드는 방법을 소개합니다.

:::hw
Board|Eageleye 530s or Rpi3 or Artik|1|Single Board Computer
Sensor|USB Camera|1|본 강의는 SPC-A1200MB를 사용하였습니다. 다른 USB카메라도 가능합니다.
Motor|SG90|2|다른 Servo Motor를 사용해도 됩니다.
Etc.|서보모터 거치대|2|시중에 판매하는 거치대를 사용하였습니다.
:::

:::sw
OS|Tizen|Tizen 5.0
SDK|SmartThings|SmartThings와 연동을 하지 않을 경우 필요없습니다
App|Smart Surveillance Camera Application|본 프로젝트의 소스코드는 다음 위치에 공개되어 있습니다 git://git.tizen.org/apps/native/smart-surveillance-camera
Etc.|IoT.js|Tizen 5.0에 포함되어 있지만, Web socket을 사용하기 위해서는 별도로 빌드한 패키지가 필요합니다
:::

# 프로젝트 구조와 동작 방식 Overview
본 프로젝트는 2개의 Application으로 구성되어 있습니다.

**카메라로 이미지를 확보하여 분석 후 카메라의 방향을 조작하는 Surveillance Camera Application**과 **Surveillance Camera Application에서 저장한 이미지를 웹 브라우저로 볼 수 있도록 제공하는 Web Server Application** 입니다. 각각의 어플리케이션은 **독립적으로 동작**하므로, 필요에 따라 구성을 변경 할 수 있습니다.

# Surveillance Camera Application
USB 카메라를 통해 확보한 연속된 이미지를 분석하여 움직임 영역을 검출합니다. **검출된 정보를 바탕으로 Servo 모터를 조향하여 움직임을 따라가는 Surveillance Camera를 구현하였습니다.** 또한 검출된 정보를 JPEG 파일의 `Exif`에 넣어서 이미지 파일을 저장하며, 이를 통해 다른 Application에서 실시간으로 촬영된 이미지를 확인 할 수 있습니다.

SmartThings 기능을 활성화 할 경우 움직임 정보를 SmartThings로 보내고, SmartThings Plugin을 통해서 Servo 모터의 방향을 직접 조향할 수 도 있습니다.

이어서 **Camera, Vision, Servo Motor, Image File, SmartThings** 부분으로 나눠서 자세하게 설명을 해보겠습니다.

## Camera로 이미지 프레임 추출
카메라를 컨트롤 하기 위해서 우선 타이젠의 카메라에 대해서 알아야 합니다.

![using_camera_states.png](https://tizen-prod-craftroom.s3.amazonaws.com/uploads/2018/08/using_camera_states.png)

위와 같은 상태를 갖으며, 상태에 따라 전환될 수 있는 상태가 제한되어 있으니 잘 알아두어야 합니다.

본 프로젝트에서는 별도로 Capture를 하지않고, Preview에서 들어오는 이미지 버퍼를 사용하기 때문에 `Capturing`과 `Captured` 상태로 변경은 없습니다.

### 카메라 준비
카메라를 사용하기 위해서 간단하게 초기화를 해줍니다. 참, `manifest`파일에서 `privilege`추가하는 것 잊으시면 안됩니다!

```xml
    <privilege>http://tizen.org/privilege/camera</privilege>
```

초기화 코드는 아래 함수에 구현되어 있습니다.

```c
    int resource_camera_init(preview_image_buffer_created_cb preview_image_buffer_created_cb, void *user_data)
```

### Preview 이미지 다루기
본 프로젝트에서는 Camera에서 연속된 Preview 이미지를 추출하여 사용합니다. **Preview Event에서 이벤트가 시작되며, 가장 중요한 부분 중 하나**입니다.

`camera_start_preview`로 `previewing` 상태가 되면 설정된 `fps`로 preview callback이 호출됩니다. 30fps로 설정되어 있을 경우 약 35ms 마다 event가 호출되며 해당 callback은 최대한 짧게 종료되는 것이 좋습니다.

이미지 분석을 하기위해 사용될 **Vision API**에서 사용할 수 있는 형태의 이미지 버퍼를 생성한 후 해당 버퍼를 전달해 주는 역할만을 수행하도록 아래와 같이 구현하였습니다.

```c
    static void __camera_preview_cb(camera_preview_data_s *frame, void *user_data)
    {
        struct __camera_data *camera_data = user_data;
        static long long int last = 0;
        long long int now = __get_monotonic_ms();

        if (now - last < CAMERA_PREVIEW_INTERVAL_MIN)
            return;

        image_buffer_data_s *image_buffer_data = __make_preview_image_buffer_data(frame);
        if (image_buffer_data == NULL) {
            _E("Failed to create mv source");
            return;
        }
        image_buffer_data->user_data = camera_data->preview_image_buffer_created_cb_data;

        ecore_main_loop_thread_safe_call_async(camera_data->preview_image_buffer_created_cb, image_buffer_data);
        last = now;
    }
```

Preview Callback이 카메라 FW에서 생성된 쓰레드에서 실행되므로 이후 동작을 main loop에서 실행하기 위해서 `ecore_main_loop_thread_safe_call_async`을 사용했습니다.

## Vision API로 움직임 검출하기
Tizen의 Media Vision API는 다양한 기능을 제공하는데, 본 프로젝트에서는 [Media Vision Surveillance](https://developer.tizen.org/development/api-references/native-application?redirect=https://developer.tizen.org/dev-guide/5.0.0/org.tizen.native.mobile.apireference/group__CAPI__MEDIA__VISION__MODULE.html)기능을 사용해서 연속된 이미지 소스에서 움직임을 검출하였습니다.

### Media Vision 설정
Media Vision API를 사용하기전에 어떤 소스를 어떻게 분석할지, 분석 결과를 어떻게 전달 받을지 등에 대해서 설정이 필요합니다. 일종의 분석 틀을 만들어 놓는 과정이라고 생각하면 됩니다.

본 프로젝트에서는 `controller_mv.c`의 `controller_mv_set_movement_detection_event_cb` 함수로 구현이 되어 있습니다.

![mvtype.jpg](https://tizen-prod-craftroom.s3.amazonaws.com/uploads/2018/11/mvtype.jpg)

중요한 부분은 위와 같이 어떤 내용을 분석할 지 설정하는 부분과 검출 이벤트가 발생하였을 경우 전달 받을 callback 등을 설정하는 부분입니다.

```c
	mv_engine_config_set_int_attribute(engine_cfg, MV_SURVEILLANCE_MOVEMENT_DETECTION_THRESHOLD, 50); /* 10 is default value [0 ~ 255] */

	ret = mv_surveillance_event_trigger_create(MV_SURVEILLANCE_EVENT_TYPE_MOVEMENT_DETECTED, &mv_data->mv_trigger_handle);
	if (ret) {
		_E("failed to mv_surveillance_event_trigger_create - [%s]", __mv_err_to_str(ret));
		goto ERROR;
	}

	ret = mv_surveillance_subscribe_event_trigger(mv_data->mv_trigger_handle, VIDEO_STREAM_ID, engine_cfg, __movement_detected_event_cb, mv_data);
```

### Media Vision에 이미지 소스 주입
위의 설정 과정에서 틀을 만들었다면, 그 다음 할 일은 카메라의 Preview callback으로부터 전달 받은 이미지 소스를 그 틀에 집어 넣는 일이 필요합니다.

아래의 callback 함수에서 `controller_mv_create_source`로 Media Vision에 맞는 이미지 소스를 생성한 다음 `controller_mv_push_source`로 이미지 소스를 집어넣고 있는 것을 볼 수 있습니다.

```c
    static void __preview_image_buffer_created_cb(void *data)
    {
...
        image_colorspace = __convert_colorspace_from_cam_to_mv(image_buffer->format);
        goto_if(image_colorspace == MEDIA_VISION_COLORSPACE_INVALID, FREE_ALL_BUFFER);

        __copy_image_buffer(image_buffer, ad);

        switch_state_get(&switch_state);
        if (switch_state == SWITCH_STATE_OFF) { /* SWITCH_STATE_OFF means automatic mode */
            source = controller_mv_create_source(image_buffer->buffer,
                        image_buffer->buffer_size, image_buffer->image_width,
                        image_buffer->image_height, image_colorspace);
        }

        pthread_mutex_lock(&ad->mutex);
        info = ad->latest_image_info;
        ad->latest_image_info = NULL;
        pthread_mutex_unlock(&ad->mutex);
        free(info);

        if (source)
            controller_mv_push_source(source);
```

특이한 점은 `controller_mv_push_source`로 이미지 소스를 집어 넣고, 움직임이 검출될 경우 곧 바로 등록된 콜백이 호출되는 `Blocking API`라는 점 입니다. 즉, `controller_mv_push_source`아래 코드는 등록된 콜백이 호출된 이후 수행이 됩니다.

### Media Vision 검출 결과 처리
틀을 만들고, 이미지 소스를 집어 넣고 나서는 분석 결과를 기다리면 됩니다. 분석 결과 움직임이 검출되지 않을 경우 callback은 호출되지 않습니다. 

**움직임이 검출될 경우** 아래의 callback 함수가 호출되며, 함수 내에서 결과를 정리해서 **Servo 모터를 조향하고 이미지 파일을 저장 할 수 있도록 설정시 등록된 callback을 호출해 줍니다.**

```c
    static void __movement_detected_event_cb(mv_surveillance_event_trigger_h trigger, mv_source_h source, int video_stream_id, mv_surveillance_result_h event_result, void *data)
    {
        int ret = 0;
        int horizontal = 0;
        int vertical = 0;
        int result[MV_RESULT_LENGTH_MAX] = {0, };
        int result_count = 0;
        int result_index = 0;
        int valid_area_sum = 0;
        int i;
        size_t move_regions_num = 0;
        mv_rectangle_s *regions = NULL;

        ret_if(!trigger);
        ret_if(!event_result);
        ret_if(!mv_data);

        ret = mv_surveillance_get_result_value(event_result, MV_SURVEILLANCE_MOVEMENT_NUMBER_OF_REGIONS, &move_regions_num);
        retm_if(ret, "failed to mv_surveillance_get_result_value for %s - [%s]", MV_SURVEILLANCE_MOVEMENT_NUMBER_OF_REGIONS, __mv_err_to_str(ret));

        regions = malloc(sizeof(mv_rectangle_s) * move_regions_num);
        ret = mv_surveillance_get_result_value(event_result, MV_SURVEILLANCE_MOVEMENT_REGIONS, regions);
        retm_if(ret, "failed to mv_surveillance_get_result_value for %s - [%s]", MV_SURVEILLANCE_MOVEMENT_REGIONS, __mv_err_to_str(ret));
```

아래의 코드는 **검출 결과를 가져와서 `Exif`에 저장하기 위한 형태의 데이터를 생성하는 부분**입니다. **`Exif`에 움직임 정보를 저장하는 이유는 저장된 이미지와 분석결과를 함께 전달하기에 가장 적당한 방법이기 때문**에 `Exif`에 저장하고, 추후 Web 브라우저에서 해당 정보를 추출해서 움직임 영역에 네모 박스를 그릴 수 있게 하였습니다.

`Exif`에 분석 결과를 저장하기 위해서 다음과 같은 형태를 정했습니다. 하나의 움직임은 8개 숫자로 구성되며 xxyywwhh 의 값을 갖는다.

xx는 0~99까지의 값을 갖는 x 상대 좌표  
yy는 0~99까지의 값을 갖는 y 상대 좌표  
ww는 0~99까지의 값을 갖는 상대 넓이  
hh는 0~99까지의 값을 갖는 상대 높이  

```c
        for (i = 0; i < move_regions_num; i++) {
            // _D("region[%u] - position[%d x %d], witdh[%d], height[%d]", i, regions[i].point.x, regions[i].point.y, regions[i].width, regions[i].height);
            // _D("region[%u] - area[%d]", i, regions[i].width * regions[i].height);

            if (regions[i].width * regions[i].height < THRESHOLD_SIZE_REGION || result_count >= MV_RESULT_COUNT_MAX)
                continue;

            result[result_index] = regions[i].point.x * 99 / IMAGE_WIDTH;
            result[result_index + 1] = regions[i].point.y * 99 / IMAGE_HEIGHT;
            result[result_index + 2] = regions[i].width * 99 / IMAGE_WIDTH;
            result[result_index + 3] = regions[i].height * 99 / IMAGE_HEIGHT;

            result_count++;
            result_index = result_count * 4;

            valid_area_sum += regions[i].width * regions[i].height;
        }
```

다음은 **Servo 모터를 조향하기 위해서 분석결과를 재가공하는 부분**입니다. 움직임의 위치와 크기에 따라 중앙에서 상하좌우로 얼마나 떨어져있는지 결정하게 되며, 여러개의 움직임이 있을 경우 값을 더해서 최종 위치를 결정하도록 하였습니다.

```c
        for (i = 0; i < move_regions_num; i++) {
            if (regions[i].width * regions[i].height < THRESHOLD_SIZE_REGION)
                continue;

            //offset 은 움직임의 중심 좌표가 화면의 중심으로 부터 얼마나 벗어났는지의 값으로 -160 ~ 160, -120 ~ 120 의 값을 갖는다.
            int x_offset = (regions[i].point.x + regions[i].width / 2) - (IMAGE_WIDTH / 2);
            int y_offset = (regions[i].point.y + regions[i].height / 2) - (IMAGE_HEIGHT / 2);
            int area = regions[i].width * regions[i].height;

            // offset 값에 움직임 크기의 상대값(비율)을 곱한 다음, 모두 더해서 최종 offset 값을 구한다.
            // 최종값의 범위는 offset 값의 범위와 같아야 한다.
            horizontal += (int) x_offset * area / valid_area_sum;
            vertical += (int) y_offset * area / valid_area_sum;
        }
        free(regions);

        mv_data->movement_detected_cb(horizontal, vertical, result, result_count, mv_data->movement_detected_cb_data);
    }
```
분석 결과를 바탕으로 움직임의 방향을 결정하는 알고리즘은 상황에 맞게 변경해서 사용하면 좋습니다.

## Servo Motor 컨트롤하기
사실 Servo Motor 컨트롤은 다른 프로젝트에서와 차이점이 별로 없습니다. **분석 결과에 따라 상하좌우로 조향하는 것**이 전부입니다만 그래도 코드를 한번 설명해 보겠습니다. :)

### PWM 신호로 Servo Motor 조향하기
Servo Motor 를 조향하기 위한 PWM 신호를 출력하는 코드는 매우 전형적인 형태이므로, 아래 코드를 포함하여, `resource_servo_motor_sg90.c` 파일 안의 코드들을 적극 활용하는 것을 추천드립니다.

```c
    int resource_set_servo_motor_sg90_value(int channel, double duty_cycle_ms)
    {
...
        if (!*pwm_h) {
            ret = peripheral_pwm_open(0, channel, pwm_h);
            if (ret != PERIPHERAL_ERROR_NONE) {
                _E("failed to open servo motor with channel(%d) : %s", channel, get_error_message(ret));
                return -1;
            }
        }

        ret = peripheral_pwm_set_period(*pwm_h, SERVO_MOTOR_DEFAULT_PERIOD * 1000 * 1000);
        if (ret != PERIPHERAL_ERROR_NONE) {
            _E("failed to set period : %s", get_error_message(ret));
            return -1;
        }

        ret = peripheral_pwm_set_duty_cycle(*pwm_h, duty_cycle_ms * 1000 * 1000);
        if (ret != PERIPHERAL_ERROR_NONE) {
            _E("failed to set duty cycle : %s", get_error_message(ret));
            return -1;
        }

        ret = peripheral_pwm_set_enabled(*pwm_h, true);
        if (ret != PERIPHERAL_ERROR_NONE) {
            _E("failed to enable : %s", get_error_message(ret));
            return -1;
        }
```

### 움직임을 따라가도록 Servo Motor 조향
**움직임의 분석 결과는 입력된 프레임 내에서의 움직임에 의한 것이므로, 최대 이동 범위로는 프레임의 가장 자리가 중앙에 들어올 수 있는 정도**가 되어야 합니다.

아래 코드에서 보면 `__move_camera`은 입력으로 -10 ~ 10의 범위를 갖는데, `STEP`은 위에서 언급한 최대 이동값을 10으로 나눈 것의 실측값입니다. 즉, **최대 10의 값을 입력 받을 경우 화면의 가장 자리가 이동 후에 화면의 중앙에 오도록** 되어 있습니다.

```c
    static void __move_camera(int x, int y, void *user_data)
    {
        app_data *ad = (app_data *)user_data;
        ret_if(!ad);

        // x, y Range : -10 ~ 10
        if (x > 10) x = 10;
        if (x < -10) x = -10;
        if (y > 10) y = 10;
        if (y < -10) y = -10;

        x *= -1; // The camera image is flipped left and right.
        double calculated_x = ad->current_servo_x + x * SERVO_MOTOR_HORIZONTAL_STEP;
        double calculated_y = ad->current_servo_y + y * SERVO_MOTOR_VERTICAL_STEP;
```

## Image File 저장하기
Image File을 저장하는 방법은 다양하지만, 본 프로젝트에서는 `Exif`를 넣고, Image Util API를 사용해서 Image Buffer를 파일로 저장하였기에 관련 부분을 간단하게 소개합니다.

### Image Util로 파일 저장하기
Image Util로 파일을 저장하는 부분은 아래 가이드에 잘 설명이 되어 있습니다. 가이드에 나와있는대로 핸들러를 만들고 Encoding / Decoding 하는 코드를 `controller_image.c`에 구현하였으니 참고하시면 됩니다.

[Image Editing](https://developer.tizen.org/development/guides/native-application/multimedia/image-editing)

### Exif에 부가 정보 넣기
Exif 에 부가 정보를 넣기 위해서 [libexif](https://libexif.github.io/)를 사용하여 User Comment 태그에 문자열을 저장하였습니다.

[libexif](https://libexif.github.io/)에 대해서 Tizen 개발자 사이트에 특별한 설명은 없지만, 해당 라이브러리의 공식 사이트의 정보를 확인하시고, 본 프로젝트에 사용된 코드를 참고하셔서 사용하시면 됩니다.

```c
    int exif_write_jpg_file_with_comment(const char *output_file,
            const unsigned char *jpg_data, unsigned int jpg_size,
            unsigned int jpg_width, unsigned int jpg_height,
            const char *comment, unsigned int comment_len)
    {
        int ret = 0;
        unsigned char *exif_data  = NULL;
        unsigned int exif_size = 0;

        if (!comment || (comment_len ==  0)) {
            _W("There is no comment");
            return save_jpeg_file(output_file, jpg_data, jpg_size);
        }

        ret = create_exif_data(jpg_data, jpg_size, jpg_width, jpg_height,
                comment, comment_len, &exif_data, &exif_size);
        if (ret) {
            _E("failed to create_exif_data(), save jpg data only");
            return save_jpeg_file(output_file, jpg_data, jpg_size);
        }

        ret = save_jpeg_file_with_exif(
                output_file, jpg_data, jpg_size, exif_data, exif_size);

        free(exif_data);
        return ret;
    }
```

## SmartThings와 연동하기
SmartThings에 프로젝트를 생성하고, 등록, 설치하는 과정은 다른 프로젝트들과 동일하기 때문에 별도로 설명하지 않겠습니다.

잘 설명되어 있는 다른 크래프트룸 프로젝트 [Smart Motion Light SmartThings](https://craftroom.tizen.org/smart-motion-light-smartthings/)를 참고하거나 [Tizen 개발자사이트](https://developer.tizen.org/) 또는 [SmartThings 개발자사이트](https://devworkspace.developer.samsung.com/smartthingsconsole/iotweb/site/#/)를 참고하시면 됩니다.

### 사용된 SmartThings Capabilities
SmartThings를 통해 Servo Motor로 카메라의 방향을 조향하고, 컨트롤 모드를 `on/off`하기 위해 아래의 Capabilities를 사용하였습니다.

```c
    // switch
    #define URI_SWITCH "/capability/switch/main/0"
    #define KEY_SWITCH "power"
    #define VALUE_SWITCH_ON "on"
    #define VALUE_SWITCH_OFF "off"

    // volume - servo v
    #define URI_AUDIOVOLUME "/capability/audioVolume/main/0"
    #define KEY_VOLUME "volume"
    #define KEY_MUTE "mute"

    // dim switch - servo h
    #define URI_SWITCHLEVEL "/capability/switchLevel/main/0"
    #define KEY_SWITCHLEVEL "dimmingSetting"

    // motion
    #define URI_MOTION "/capability/motionSensor/main/0"
    #define KEY_MOTION "value"
```

각각의 Capabilities는 값을 읽고 쓰는 기본적인 동작으로 사용하였기에 내용이 복잡하지 않으며, 어떻게 사용하였는지는 코드를 통해서 확인해 보실 수 있습니다.

### SmartThings Custom Plugin
SmartThings에서 사용한 Capabilities 들이 본 프로젝트의 내용과 일치하지 않는 부분이 있기에, [SmartThings Device Plugin 가이드](https://smartthings.developer.samsung.com/docs/guides/plugin/overview.html)를 따라 플러그인을 만들었습니다.

![screenshot_20181122-135920-horz](https://user-images.githubusercontent.com/44985886/48883849-081c9a80-ee65-11e8-982f-65f42f5541bd.jpg)

플러그인 프로젝트는 `smartthings-plugin` 폴더에 있으면 가이드를 따라서 타겟 디바이스에 설치하시면 됩니다.

# Web Server Application
Web Server Application은 IoT.js를 이용해서 web server를 구현한 것입니다. 브라우저 상에서 본 프로젝트의 내용을 확인 할 수 있는 static 파일들(html, css, js)을 배포하고, `Web Socket`을 통해서 실시간으로 현재 캡쳐된 이미지 파일을 Push 하는 역할을 합니다.

## IoTjs 준비하기
`IoTjs`는 `JerryScript`라는 Javascript 엔진을 사용하여 만들어진 IoT Framework 입니다. 쉽게 비유하자면 IoT를 위한 nodejs 같은 것이라고 생각하셔도 좋을것 같습니다.

이미 Tizen 5.0 에 포함되어 있으며 별다른 준비가 필요없지만, 5.0에 포함된 버전은 Web socket 기능이 꺼진채로 빌드된 버전이라 Web socket을 사용하기 위해서는 별도의 빌드가 필요합니다. 그래서 아래 `/RPMS/IoTjs` 폴더에 빌드해서 넣어 두었습니다.

아래 커맨드로 타겟 디바이스에 설치해 주시면 됩니다.

```bash
    $sdb root on
    $sdb push ./RPMS/IoTjs/iotjs-1.0.0-99.armv7l.rpm /tmp
    $sdb shell 'mount -o remount,rw /'
    $sdb shell 'rpm -ivh --force /tmp/iotjs-1.0.0-99.armv7l.rpm'
```

## IoTjs 웹서버 만들기
