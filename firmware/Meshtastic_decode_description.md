# Описание декодирования пакетов в Meshtastic (папка mesh_src)

## Основной поток обработки пакетов

1. **Получение пакета**: Пакеты принимаются через RadioInterface (например, SX126x, SX128x и т.д.).
2. **Обработка в Router**: Все принятые пакеты обрабатываются в классе `Router` в файле `src/mesh/Router.cpp`.
3. **Декодирование**: Функция `perhapsDecode(meshtastic_MeshPacket *p)` в Router.cpp отвечает за декодирование пакетов.

## Ключевые компоненты декодирования

### Структура пакета Meshtastic

- **Заголовок** (PacketHeader): ID, From, To, Flags, Channel hash
- **Payload**: Зашифрованные данные (используются protobuf для сериализации)
- Протокол использует AES-CTR для шифрования и HMAC для аутентификации

### Функция perhapsDecode()

Расположена в `src/mesh/Router.cpp` (строки ~340-420).

Этапы декодирования:

1. **Проверка уже декодированного пакета**:
   ```cpp
   if (p->which_payload_variant == meshtastic_MeshPacket_decoded_tag)
       return DecodeState::DECODE_SUCCESS;
   ```

2. **PKI декодирование (если применимо)**:
   - ИспользуетCurve25519 для ECDH
   - Проверяет публичные ключи отправителя/получателя
   - Декодирует protobuf данные из `meshtastic_Data`

3. **Стандартное декодирование**:
   - Перебирает каналы для поиска подходящего ключа
   - Расшифровывает AES-CTR
   - Десериализует protobuf с помощью `pb_decode_from_bytes(bytes, rawSize, &meshtastic_Data_msg, &decodedtmp)`
   - Проверяет корректность protobuf и portnum

4. **Дополнительная обработка**:
   - Устанавливает индекс канала вместо хэша
   - Обрабатывает bitfield (want_response и др.)
   - Логирует декодированный пакет

### Десериализация protobuf

Используется библиотека Nanopb для декодирования:

```cpp
meshtastic_Data decodedtmp;
memset(&decodedtmp, 0, sizeof(decodedtmp));
if (pb_decode_from_bytes(bytes, rawSize, &meshtastic_Data_msg, &decodedtmp)) {
    p->decoded = decodedtmp;
    p->which_payload_variant = meshtastic_MeshPacket_decoded_tag;
    decrypted = true;
}
```

### Обработка после декодирования

- Пакет передается в модули через `MeshModule::callModules(*p, src)`
- Модули определяют, что делать с пакетом на основе `portnum` (например, TEXT_MESSAGE_APP, POSITION_APP и т.д.)
- Отправка через MQTT, UDP или дальнейшая маршрутизация

## Необходимые файлы

- `src/mesh/Router.cpp` - основная логика декодирования
- `src/mesh/CryptoEngine.cpp` - криптографические функции
- `src/mesh/Channels.cpp` - управление каналами
- `src/mesh/mesh-pb-constants.cpp` - protobuf константы
- `src/mesh/generated/` - сгенерированные protobuf файлы


