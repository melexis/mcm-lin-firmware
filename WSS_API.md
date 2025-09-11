# WebSocket API Description

```json
{
  "id": <string>,           // optional
  "type": <string>,         // info|command|ack|error
  "payload": { ... }        // optional, message specific data
}
```

## Information Type

Request

```json
{
  "id": "my_message_id",
  "type": "info"
}
```

Response

```json
{
  "id": "my_message_id",
  "type": "ack",
  "payload": {
    "api_rev": 2,
    "model": "Melexis Compact Master LIN",
    "firmware_version": <string>
  }
}
```

## Command Type

Request

```json
{
  "id": "my_message_id",
  "type": "command",
  "payload": {
    "endpoint": <string>,
    "command": <string>,
    "params": { ... }
  }
}
```

Response

```json
{
  "id": "my_message_id",
  "type": "ack",
  "payload": { ... }
}
```

```json
{
  "id": "my_message_id",
  "type": "error",
  "payload": {
    "message": <string>
  }
}
```

### Bootloader

#### Program Memory

Request

```json
{
  "id": "my_message_id",
  "type": "command",
  "payload": {
    "endpoint": "bootloader",
    "command": "program",
    "params": {
      "hexfile": <string>,
      "memory": <string>,
      "manpow": <boolean>,
      "bitrate": <number>,
      "fullduplex": <boolean>,
      "txpin": <number>,
      "flashkeys": <array of numbers>
    }
  }
}
```

Memory key can have values `flash`, `nvram` or `eeprom`.

Response

```json
{
  "id": "my_message_id",
  "type": "ack",
  "payload": { }
}
```

#### Verify Memory

Request

```json
{
  "id": "my_message_id",
  "type": "command",
  "payload": {
    "endpoint": "bootloader",
    "command": "verify",
    "params": {
      "hexfile": <string>,
      "memory": <string>,
      "manpow": <boolean>,
      "bitrate": <number>,
      "fullduplex": <boolean>,
      "txpin": <number>,
      "flashkeys": <array of numbers>
    }
  }
}
```

Memory key can have values `flash`, `nvram` or `eeprom`.

Response

```json
{
  "id": "my_message_id",
  "type": "ack",
  "payload": { }
}
```

### Power Output

#### Control

Request

```json
{
  "id": "my_message_id",
  "type": "command",
  "payload": {
    "endpoint": "power_out",
    "command": "control",
    "params": {
      "switch_enable": <boolean>
    }
  }
}
```

Response

```json
{
  "id": "my_message_id",
  "type": "ack",
  "payload": { }
}
```

#### Status

Request

```json
{
  "id": "my_message_id",
  "type": "command",
  "payload": {
    "endpoint": "power_out",
    "command": "status"
  }
}
```

Response

```json
{
  "id": "my_message_id",
  "type": "ack",
  "payload": {
    "switch_enabled": <boolean>
  }
}
```

### System

#### Wifi

Request

```json
{
  "id": "my_message_id",
  "type": "command",
  "payload": {
    "endpoint": "system",
    "command": "wifi"
  }
}
```

Response

```json
{
  "id": "my_message_id",
  "type": "ack",
  "payload": {
    "link_up": <boolean>,
    "ip": <number>,
    "netmask": <number>,
    "gateway": <number>,
  }
}
```

#### Identify

TBD

#### Reboot

TBD

### LIN

TODO

## Connection Alive Check

Request

```json
{
  "__ping__": true
}
```

Response

```json
{
  "__pong__": true
}
```

