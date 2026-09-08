# Unreal Engine и MCP

Настроено и проверено 8 сентября 2026 года на macOS.

## Установленное окружение

- Unreal Engine **5.8.2**: `/Users/Shared/Epic Games/UE_5.8`.
- Проект: `/Users/shaman/Desktop/Projects/Unreal/Altai/Altai.uproject`.
- Используется встроенный сервер Epic **Unreal MCP**, идентификатор плагина `ModelContextProtocol`. Сторонний сервер не устанавливался.
- В проекте включены `ModelContextProtocol`, `AllToolsets` и `PythonScriptPlugin`, только для Editor target. Зависимости подключаются движком.
- Плагин Epic имеет статус Experimental. Перед обновлением движка повторять проверку совместимости; в выпущенную игру редакторские инструменты не включать.

## Соединение

Адрес: `http://127.0.0.1:8000/mcp`, Streamable HTTP.

Автозапуск настроен в `Config/DefaultEditorPerProjectUserSettings.ini`. Привязка HTTP listener к `127.0.0.1` задана в `Config/DefaultEngine.ini`. Сервер работает, пока открыт редактор этого проекта. Порт не открывается для локальной сети.

В общей конфигурации Codex `~/.codex/config.toml` зарегистрирован сервер `unreal`:

```toml
[mcp_servers.unreal]
url = "http://127.0.0.1:8000/mcp"
```

Повторная регистрация на другом компьютере:

```sh
codex mcp add unreal --url http://127.0.0.1:8000/mcp
```

Пути и установленную версию движка на другом компьютере проверить отдельно. Не запускать одновременно два проекта с MCP на одном порту.

## Обычный запуск

1. Открыть `Altai.uproject` в Unreal Engine 5.8.
2. Дождаться загрузки редактора и запуска MCP.
3. Подключить/обновить MCP в клиенте Codex. Если уже открытая сессия не подхватила инструменты, перезапустить клиент после запуска редактора.
4. Вызвать `list_toolsets`, затем `describe_toolset` для нужного набора, затем `call_tool` с полученной схемой аргументов.

Вызовы редактора выполнять последовательно. Для ручного запуска в консоли Unreal есть `ModelContextProtocol.StartServer`; для обновления списка — `ModelContextProtocol.RefreshTools`.

## Что проверено

- Реальный редактор Altai открылся; журнал: `~/Library/Logs/Unreal Engine/AltaiEditor/Altai.log`.
- Listener слушает именно `127.0.0.1:8000`.
- MCP `initialize` и `tools/list` успешно ответили.
- `list_toolsets` вернул наборы работы с уровнями, акторами, Blueprint, материалами, UI, PCG, Niagara и другими областями.
- Через MCP выполнены `SceneTools.get_current_level` и `EditorAppToolset.GetSelectedActors`: получены `/Temp/Untitled_1` и пустой список выбранных акторов. Это ожидаемо для нового пустого проекта.
- `codex mcp get unreal` подтверждает включённое HTTP-подключение. Прямой протокольный тест выполнен отдельно от текущего каталога инструментов беседы; появление инструментов в уже открытом клиенте может потребовать переподключения.
- После повторного запуска автозапуск и чтение текущего уровня через MCP проверены снова. Для зависимости GameFeatures добавлено правило `GameFeatureData` в `Config/DefaultGame.ini`; ошибка отсутствующего правила после перезапуска устранена.

В журнале установленной сборки при старте остаются 15 сообщений `LogAutomationTest: Error: Condition failed`. Их причина отдельно не установлена; редактор и проверенные MCP-вызовы работают. Это не результаты тестов Altai и не основание считать всё окружение проверенным без ошибок.

Эта первоначальная проверка подтверждала только связь с редактором. Позднее созданный игровой каркас, его ассеты и проверки описаны в [PROTOTYPE_STATUS](PROTOTYPE_STATUS.md); лаборатория пока не реализована.

## Источники

- [Epic: Unreal MCP в Unreal Engine 5.8](https://dev.epicgames.com/documentation/unreal-engine/unreal-mcp-in-unreal-editor?application_version=5.8).
- [OpenAI: подключение MCP к Codex](https://learn.chatgpt.com/docs/extend/mcp?surface=cli).
- Фактические имена параметров сверены с `ModelContextProtocolSettings.h` установленного движка.
