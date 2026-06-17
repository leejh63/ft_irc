# 설계 문서

## 목표

`ft_irc`는 C++98로 작성한 단일 스레드 IRC 서버입니다. 서버는 하나의 `poll()` 루프에서 새 연결, 수신, 송신, 연결 종료를 모두 처리합니다. listening socket과 client socket은 모두 non-blocking file descriptor로 설정합니다.

네트워크 입출력과 IRC 상태 변경 로직은 분리되어 있습니다. `Server`는 socket과 buffer를 관리하고, `IrcCore`는 파싱된 IRC 명령을 처리합니다. 명령 처리 결과는 `ServerAction`으로 반환되며, 실제 전송과 연결 종료는 다시 `Server`에서 수행합니다.

## 전체 구조

```text
main
  -> Server
      -> SocketMonitor
      -> IrcParser
      -> IrcCore
          -> ClientRegistry
          -> ChannelRegistry
          -> IrcMessageBuilder
```

각 모듈의 역할은 다음과 같습니다.

```text
Server              socket 생성, accept, recv, send, close, buffer 관리
SocketMonitor       pollfd 목록 관리
IrcParser           IRC line 파싱
IrcCore             IRC 명령 처리와 상태 변경
ClientRegistry      클라이언트별 등록 상태와 입출력 buffer 저장
ChannelRegistry     채널별 member, operator, mode, topic 저장
IrcMessageBuilder   numeric 응답과 broadcast 메시지 생성
```

## `Server`

`Server`는 네트워크 계층을 담당합니다.

주요 역할:

- IPv4 TCP listening socket 생성
- `bind()`, `listen()`, `accept()` 처리
- listening socket과 client socket을 non-blocking으로 설정
- 하나의 `poll()` 루프 실행
- 클라이언트별 input buffer와 output buffer 관리
- 수신 byte를 input buffer에 누적
- 완성된 IRC line을 추출해 `IrcCore`에 전달
- `IrcCore`가 반환한 `ServerAction` 처리
- 보낼 message를 output buffer에 누적
- fd가 writable 상태일 때만 `send()` 수행
- 연결 종료 시 poll 목록과 registry에서 클라이언트 제거

`Server`는 IRC 명령의 의미를 직접 판단하지 않습니다. 명령의 의미와 상태 변경은 `IrcCore`가 담당합니다.

## `SocketMonitor`

`SocketMonitor`는 `poll()`에 전달할 `pollfd` 목록을 관리합니다.

주요 역할:

- listening fd와 client fd를 하나의 목록으로 유지
- 기본 감시 이벤트로 `POLLIN` 설정
- 보낼 데이터가 있는 client fd에 `POLLOUT` 설정
- output buffer가 비면 `POLLOUT` 해제
- 닫힌 fd를 목록에서 제거

## `IrcParser`

`IrcParser`는 수신한 IRC line을 명령 처리에 사용할 수 있는 형태로 분리합니다.

파싱 결과:

- command verb
- 일반 parameter 목록
- trailing parameter
- trailing parameter 존재 여부
- 원본 line

`TOPIC #room`과 `TOPIC #room :`은 의미가 다르기 때문에 trailing parameter의 존재 여부를 별도로 보관합니다.

## `IrcCore`

`IrcCore`는 IRC 명령 처리를 담당합니다.

주요 역할:

- 등록 상태 확인
- 명령어별 handler 호출
- 채널 입장 여부 확인
- 채널 operator 권한 확인
- 클라이언트와 채널 상태 변경
- numeric 응답 생성
- channel broadcast 메시지 생성
- `QUIT`과 연결 종료 흐름에서 필요한 정리 작업 요청

`IrcCore`는 socket에 직접 쓰지 않습니다. 응답은 `ServerAction`으로 만들어 `Server`에 전달합니다.

## `ClientRegistry`

`ClientRegistry`는 fd 기준으로 클라이언트 상태를 저장합니다.

관리 정보:

- input buffer
- output buffer
- password 인증 여부
- nickname 설정 여부
- username 설정 여부
- 등록 완료 여부
- nickname
- username
- realname
- user mode

nickname 비교에는 IRC 방식의 대소문자 접힘 규칙을 적용합니다. 따라서 `alice`와 `Alice`는 같은 nickname으로 처리됩니다.

## `ChannelRegistry`

`ChannelRegistry`는 채널 이름 기준으로 채널 상태를 저장합니다.

관리 정보:

- 멤버 목록
- channel operator 목록
- invite 목록
- topic
- 초대 전용 모드
- 토픽 보호 모드
- 채널 키
- 인원 제한

채널에 멤버가 남아 있는 동안 operator 목록이 비어 있지 않도록 관리합니다. 기존 operator가 채널을 떠난 경우 남은 멤버 중 한 명이 operator 역할을 이어받습니다.

## `IrcMessageBuilder`

`IrcMessageBuilder`는 IRC 응답 문자열을 생성합니다.

사용 범위:

- numeric 응답
- 사용자 접두 형식
- welcome 응답
- channel broadcast
- `JOIN`, `PART`, `PRIVMSG`, `TOPIC`, `INVITE`, `KICK`, `MODE`, `QUIT` 메시지

응답 문자열 생성 위치를 한 곳으로 모아 protocol 출력 형식을 일정하게 유지합니다.

## 요청 처리 흐름

```text
client fd에 POLLIN 발생
  -> Server::recv()
  -> ClientEntry::inBuf에 byte 누적
  -> \n 기준으로 IRC line 추출
  -> IrcParser::parse_Line()
  -> IrcCore::handle_Command()
  -> registry 상태 변경
  -> ServerAction 생성
  -> Server가 ClientEntry::outBuf에 응답 추가
  -> 보낼 데이터가 있으면 POLLOUT 활성화
  -> fd가 writable이면 Server::send() 수행
```

## 등록 흐름

```text
PASS 성공
  -> passOk = true
NICK 설정
  -> hasNick = true
USER 설정
  -> hasUser = true
PASS / NICK / USER 조건 충족
  -> registered = true
  -> welcome 응답 전송
```

등록이 완료되기 전에는 등록이 필요한 명령을 사용할 수 없습니다.

## 채널 정리 흐름

```text
클라이언트가 PART / QUIT / disconnect / KICK으로 채널에서 제거됨
  -> 멤버 목록에서 fd 제거
  -> operator 목록에서 fd 제거
  -> invite 목록에서 fd 제거
  -> 채널이 비면 channel 삭제
  -> 멤버가 남아 있고 operator가 없으면 남은 멤버 중 한 명을 operator로 지정
```

## 버퍼링 규칙

- 입력은 `\n`이 들어올 때까지 input buffer에 누적합니다.
- 추출된 line 끝에 `\r`이 있으면 제거합니다.
- input buffer 또는 output buffer가 허용 범위를 넘으면 연결 종료 대상으로 처리합니다.
- `send()`가 일부만 전송하면 남은 데이터는 output buffer에 유지합니다.
- fd가 writable 상태일 때만 output buffer를 전송합니다.
