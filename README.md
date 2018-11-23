# 해카톤 진행에 도움이 될 Tizen IoT Recipe

## Smart Surveillance Camera
카메라로 움직임을 감지하여 카메라 방향을 전환하는 프로젝트입니다.

[해당 프로젝트 크래프트룸 바로가기](https://craftroom.tizen.org/%ec%95%84%ed%8b%b1%ec%9c%bc%eb%a1%9c-smart-surveillance-camera-%eb%a7%8c%eb%93%a4%ea%b8%b0/)

원본 소스코드는 아래 git repository에서 다운로드 가능  
git://git.tizen.org/apps/native/smart-surveillance-camera  
복사본은 [smart-surveillance-camera](./smart-surveillance-camera) 폴더에...

![smart-camera](https://tizen-prod-craftroom.s3.amazonaws.com/uploads/2018/11/18503b64-e758-11e8-9e15-d6d14ff41f9e-1.jpg)

### 특징
- **Camera 사용**, 카메라로 이미지를 캡쳐
- **Vision API 사용**, Vision API로 이미지 분석
- **IoT.js 사용**, IoT.js로 웹서버 구동
- **PWM 사용**, PWM 신호로 서보모터를 조향
- **SmartThing 연동**, 연동하는 시나리오 포함

## How to use analog CO2 Sensor (Korean)
CO2 센서의 CO2 측정값을 읽어오는 프로젝트입니다.

[해당 프로젝트 크래프트룸 바로가기](https://craftroom.tizen.org/co2/)

원본 소스코드는 아래 git repository에서 다운로드 가능  
git://git.tizen.org/apps/native/st-things-co2-meter  
복사본은 st-things-co2-meter 폴더에...

![co2_app](https://user-images.githubusercontent.com/44985886/48754045-a842ba00-ecd3-11e8-9617-e8e51b7d5e88.jpg)

### 특징
- **ADC칩 사용**, CO2 센서의 아날로그 연결을 위해 `MCP3008`이라는 ADC 칩을 사용
- **`Bread Board`를 사용**, `MCP3008`에 각종 핀을 연결하기 위해
- **SPI API사용**, `MCP3008`과 SPI(Serial Peripheral Interface)으로 데이터 교환

## Illuminance Sensor to Servo Motor
조도 센서의 측정값을 읽어와서 서보 모터를 컨트롤하는 프로젝트입니다.

[해당 프로젝트 크래프트룸 바로가기](https://craftroom.tizen.org/illuminance_to_servo/)

원본 소스코드는 아래 git repository에서 다운로드 가능  
git://git.tizen.org/apps/native/st-things-blind  
복사본은 st-things-blind 폴더에...

![illuminance](https://user-images.githubusercontent.com/44985886/48767352-ece14c00-ecf9-11e8-9cc2-0170c8717ad5.jpg)

### 특징
- **I2C 통신 사용**, 조도 센서와 I2C로 통신을 통해 데이터 교환
- **PWM 사용**, PWM 신호로 서보모터를 조향
- **SmartThing 연동**, 연동하는 시나리오 포함

## Smart Motion Light Basic
모션 센서의 측정값을 읽어와서 움직임이 감지될 때 LED을 켜는 프로젝트입니다.

[해당 프로젝트 크래프트룸 바로가기](https://craftroom.tizen.org/smartmotionlight/)

원본 소스코드는 아래 git repository에서 다운로드 가능  
git://git.tizen.org/apps/native/st-things-light  
복사본은 st-things-light 폴더에...

![motion](https://tizen-prod-craftroom.s3.amazonaws.com/uploads/2018/08/title_smart_motion_light_basic.png)

### 특징
- **GPIO 사용**, GPIO 값을 읽어 모션 센서의 움직임 감지 확인 및 LED 컨트롤
- **SmartThing 연동**, 연동하는 시나리오 포함
