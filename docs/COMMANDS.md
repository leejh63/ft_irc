# 명령어 동작 정리

이 문서는 서버가 처리하는 IRC 명령의 동작 범위를 정리합니다. 명령어 이름, numeric 응답, mode 문자는 IRC 프로토콜 표기를 유지합니다.

## 등록 명령어

### `PASS <password>`

서버 비밀번호를 확인합니다. `NICK`과 `USER`가 등록 절차에 사용되기 전에 올바른 `PASS`가 먼저 처리되어야 합니다.

잘못된 비밀번호가 들어오면 비밀번호 인증 상태를 해제하여 등록이 이어지지 않도록 합니다.

### `NICK <nickname>`

클라이언트의 nickname을 설정하거나 변경합니다.

닉네임 중복 검사는 IRC 방식의 대소문자 접힘 규칙을 적용합니다. 따라서 `alice`와 `Alice`는 같은 닉네임으로 취급됩니다.

이미 사용 중인 닉네임이면 numeric 응답으로 거절합니다.

### `USER <username> 0 * :<realname>`

username과 realname을 설정합니다.

`PASS`, `NICK`, `USER`가 모두 완료되면 등록 완료 상태가 되고 welcome 응답을 보냅니다.

## 채널 명령어

### `JOIN #channel [key]`

채널에 입장합니다. 채널이 없으면 새로 생성합니다. 새 채널의 첫 번째 멤버는 채널 operator가 됩니다.

입장 전에 다음 조건을 확인합니다.

- 초대 전용 모드가 켜져 있는지
- 채널 키가 필요한지
- 인원 제한에 도달했는지
- 이미 채널에 들어와 있는지

입장이 완료되면 `JOIN` 메시지를 전파하고, topic 응답과 names 응답을 보냅니다.

### `PART #channel [:reason]`

채널에서 나갑니다.

요청 클라이언트가 해당 채널의 멤버인지 확인한 뒤 `PART` 메시지를 채널 멤버에게 전파합니다. 클라이언트가 나간 뒤 채널이 비면 채널을 제거합니다.

### `PRIVMSG <target> :<text>`

사용자 또는 채널로 메시지를 보냅니다.

대상이 사용자이면 해당 nickname의 클라이언트에게 메시지를 전달합니다. 대상이 채널이면 sender가 채널 멤버인지 확인한 뒤 sender를 제외한 채널 멤버에게 메시지를 전달합니다.

### `TOPIC #channel [:topic]`

채널 topic을 조회하거나 변경합니다.

- trailing parameter가 없으면 현재 topic을 조회합니다.
- trailing parameter가 있으면 topic을 설정합니다.
- `TOPIC #channel :`은 topic을 빈 값으로 설정합니다.

채널에 `+t` mode가 설정되어 있으면 채널 operator만 topic을 변경할 수 있습니다.

### `INVITE <nickname> #channel`

사용자를 채널로 초대합니다.

요청자는 해당 채널의 멤버여야 하며 채널 operator 권한이 필요합니다. 초대가 완료되면 대상 클라이언트에게 `INVITE` 메시지를 보냅니다.

### `KICK #channel <nickname> [:reason]`

사용자를 채널에서 제거합니다.

요청자는 해당 채널의 멤버여야 하며 채널 operator 권한이 필요합니다. 대상 사용자가 채널에 있으면 `KICK` 메시지를 전파하고 채널 멤버 목록에서 제거합니다.

### `QUIT [:reason]`

클라이언트 연결을 종료합니다.

종료하는 클라이언트와 하나 이상의 채널을 공유하는 다른 클라이언트에게 `QUIT` 메시지를 보냅니다.

## 채널 모드

하나의 채널 모드 문자열에 여러 변경이 포함될 수 있습니다. 앞에서 처리된 유효한 mode는 적용 및 전달 대상에 포함되며, 뒤에서 유효하지 않은 항목이 나오면 이미 처리된 mode를 채널에 알린 뒤 해당 항목에 대한 numeric 응답을 보냅니다.

### `+i` / `-i`

초대 전용 모드를 켜거나 끕니다.

`+i` 상태에서는 초대받은 클라이언트만 채널에 입장할 수 있습니다.

### `+t` / `-t`

topic protection mode를 켜거나 끕니다.

`+t` 상태에서는 채널 operator만 topic을 변경할 수 있습니다.

### `+k <key>` / `-k`

채널 키를 설정하거나 제거합니다.

key가 설정된 채널에 입장하려면 `JOIN #channel <key>` 형식으로 올바른 key를 전달해야 합니다.

### `+o <nickname>` / `-o <nickname>`

대상 사용자에게 채널 operator 권한을 부여하거나 제거합니다.

대상 사용자는 해당 채널의 멤버여야 합니다. 마지막 operator를 제거하는 요청은 거부됩니다.

### `+l <limit>` / `-l`

채널 인원 제한을 설정하거나 제거합니다.

`+l`에 사용되는 limit 값은 양수여야 합니다.

## 사용자 모드

요청자 자신의 nickname을 대상으로 하는 간단한 user mode 처리를 지원합니다.

지원하는 mode:

- `+i`
- `-i`
- `+w`
- `-w`

## 클라이언트 호환 명령어

### `PING [:token]`

전달받은 token을 사용해 `PONG`으로 응답합니다.

### `PONG`

추가 동작 없이 수신 처리합니다.

### `CAP LS`, `CAP LIST`, `CAP REQ`, `CAP END`

일반 IRC 클라이언트가 접속 과정에서 사용하는 기본 capability negotiation 흐름을 처리합니다.

### `WHO #channel`

채널 멤버 정보를 보낸 뒤 end-of-WHO 응답을 보냅니다.
