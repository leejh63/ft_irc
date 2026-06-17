# 사용 예시

아래 예시는 원시 IRC 명령으로 서버 동작을 살펴볼 때 사용할 수 있는 기본 흐름입니다.

서버 실행:

```bash
make
./ircserv 6667 pass123!
```

각 클라이언트는 다음 방식으로 접속할 수 있습니다.

```bash
nc -C 127.0.0.1 6667
```

## 1. 등록

클라이언트 A:

```text
PASS pass123!
NICK alice
USER alice 0 * :Alice Example
```

동작 흐름:

```text
001 alice :Welcome to the Internet Relay Network alice
```

## 2. 채널 입장과 이름 목록 응답

클라이언트 A:

```text
JOIN #room
```

동작 흐름:

- `alice`가 `#room`에 입장합니다.
- 첫 번째 멤버는 이름 목록 응답에서 채널 operator로 표시됩니다.
- 토픽이 없으면 토픽 없음 응답을 받습니다.
- 이름 목록 응답에 채널 멤버 목록이 표시됩니다.

## 3. 두 번째 클라이언트 입장과 채널 메시지

클라이언트 B:

```text
PASS pass123!
NICK bob
USER bob 0 * :Bob Example
JOIN #room
```

클라이언트 A:

```text
PRIVMSG #room :hello bob
```

동작 흐름:

- 클라이언트 B는 클라이언트 A가 보낸 채널 메시지를 받습니다.
- 클라이언트 A는 자신이 보낸 채널 `PRIVMSG`를 다시 받지 않습니다.

## 4. 사용자 직접 메시지

클라이언트 B:

```text
PRIVMSG alice :hello alice
```

동작 흐름:

- 클라이언트 A는 클라이언트 B가 보낸 직접 메시지를 받습니다.

## 5. 토픽 처리

클라이언트 A:

```text
TOPIC #room :project topic
TOPIC #room
TOPIC #room :
```

동작 흐름:

- 토픽 설정 메시지가 채널 멤버에게 전달됩니다.
- 토픽 조회 시 현재 토픽을 받습니다.
- 빈 trailing parameter는 토픽을 비우는 요청으로 처리됩니다.

## 6. 초대 전용 채널

클라이언트 A:

```text
MODE #room +i
```

클라이언트 C:

```text
PASS pass123!
NICK charlie
USER charlie 0 * :Charlie Example
JOIN #room
```

동작 흐름:

- 초대받지 않은 클라이언트 C는 `#room`에 입장할 수 없습니다.

클라이언트 A:

```text
INVITE charlie #room
```

클라이언트 C:

```text
JOIN #room
```

동작 흐름:

- 초대 이후 클라이언트 C는 `#room`에 입장할 수 있습니다.

## 7. 채널 키와 인원 제한

클라이언트 A:

```text
MODE #room +k secret
MODE #room +l 3
```

동작 흐름:

- 이후 입장하는 클라이언트는 `JOIN #room secret` 형태로 key를 제공해야 합니다.
- 채널 인원이 limit에 도달하면 추가 입장이 거부됩니다.

## 8. 운영자 권한과 강제 퇴장

클라이언트 A:

```text
MODE #room +o bob
```

클라이언트 B:

```text
KICK #room charlie :bye
```

동작 흐름:

- 클라이언트 B는 운영자 권한을 받은 뒤 `KICK`을 사용할 수 있습니다.
- 채널 멤버는 강제 퇴장 메시지를 받습니다.
- 강제 퇴장 대상 클라이언트는 채널에서 제거됩니다.

## 9. 닉네임 대소문자 처리

클라이언트 A가 이미 `alice`로 등록되어 있는 상태입니다.

클라이언트 D:

```text
PASS pass123!
NICK Alice
USER Alice 0 * :Case Example
```

동작 흐름:

- `alice`가 이미 사용 중이므로 `Alice`는 중복 닉네임으로 처리됩니다.

## 10. 운영자 유지

새 채널에서 진행합니다.

클라이언트 A:

```text
JOIN #continuity
```

클라이언트 B:

```text
JOIN #continuity
```

클라이언트 A:

```text
PART #continuity
```

클라이언트 B:

```text
MODE #continuity +t
```

동작 흐름:

- 클라이언트 A가 나간 뒤에도 클라이언트 B가 채널을 관리할 수 있습니다.

## 11. 여러 채널 모드 처리

클라이언트 A가 `#room`의 운영자인 상태입니다.

클라이언트 A:

```text
MODE #room +to ghost
```

동작 흐름:

- `+t`는 채널에 적용되고 채널 멤버에게 전달됩니다.
- 등록되지 않은 `ghost`에 대한 `+o` 요청은 대상 닉네임 없음 응답을 받습니다.
- `MODE #room` 조회 시 토픽 보호 모드가 켜져 있습니다.
