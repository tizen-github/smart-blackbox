# 해카톤 진행에 도움이 될 Tizen IoT Recipe

## Smart Surveillance Camera
Tizen Vision API, Camera API, Peripheral I/O API 를 사용하여 움직임을 감지하여 카메라 방향을 전환하는 프로젝트입니다.
Camera로 캡쳐한 이미지를 Vision API로 분석하고, 분석 결과를 바탕으로 Peripheral I/O API를 이용해 PWM 신호를 출력하여 Servo 모터를 조향합니다.

https://craftroom.tizen.org/smart-surveillance-camera/

## [How to use analog CO2 Sensor (Korean)](https://craftroom.tizen.org/co2/)
Peripheral I/O API를 이용하여 CO2 센서의 CO2 측정값을 읽어오는 프로젝트입니다.
![co2_app](https://user-images.githubusercontent.com/44985886/48754045-a842ba00-ecd3-11e8-9617-e8e51b7d5e88.jpg)

### 특징
- **ADC칩 사용**, CO2 센서의 아날로그 연결을 위해 `MCP3008`이라는 ADC 칩을 사용
- **`Bread Board`를 사용**, `MCP3008`에 각종 핀을 연결하기 위해
- **SPI API사용**, `MCP3008`과 SPI(Serial Peripheral Interface)으로 데이터 교환

## Illuminance Sensor to Servo Motor
Peripheral I/O API를 이용하여 조도 센서의 측정값을 읽어와서 서보 모터를 컨트롤하는 프로젝트입니다.
조도 센서와는 I2C 통신을 하고, 서보 모터는 PWM 신호를 입력하여 조향합니다.

https://craftroom.tizen.org/illuminance_to_servo/

## 간단한 IoT Camera 어플리케이션 만들어보기
USB Camera를 이용하여 간단한 IoT 카메라를 구성하는 프로젝트입니다.
USB 카메라만 사용하며, 주기적으로 이미지 파일을 저장하는 간단한 프로젝트 입니다.

https://craftroom.tizen.org/simple-iot-camera/

## Smart Motion Light Basic
Peripheral I/O API를 이용하여 모션 센서의 측정값을 읽어와서 움직임이 감지될 때 LED을 켜는 프로젝트입니다.
GPIO 핀의 값을 읽어 모션 센서의 움직임 감지 이벤트를 전달 받습니다.

https://craftroom.tizen.org/smartmotionlight/
