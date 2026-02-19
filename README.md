- **ConfiguratorModel**  
  Централизованная модель конфигурации для PluginCore-экосистемы: регистрация конфигов, автосоздание `./configs/<name>.json`, загрузка/сохранение и генерация JSON-схемы (skeleton) для UI.

- **BaseConfig API**  
  Минимальный типобезопасный слой конфигурации:  
  - `d3156::Config` как корневой узел/контейнер  
  - `ConfigField<T>` для полей (`string/bool/uint/enum`)  
  - `ConfigArray<T>` для массивов значений  
  - макросы `CONFIG_STRING/CONFIG_BOOL/CONFIG_UINT/CONFIG_ENUM` для декларативного описания конфигов.

- **ConfiguratorPlugin**  
  Web-плагин для управления конфигурациями:  
  - HTTP endpoints для чтения/записи текущих конфигов и получения схемы  
  - Страница UI (embedded HTML) + отдельная login-страница  
  - Endpoint `./reload` для перезапуска через `SIGINT`.

- **TOTP-аутентификация**  
  Лёгкая схема авторизации на базе:  
  - генерации секрета (`ConfiguratorSecrets`) при первом запуске  
  - TOTP (6 digits) с шагом 15 секунд и окном -1/0/+1  
  - проверки `SHA256(username + totp + salt)` в заголовке `Authorization`.

- **GitLab CI/CD pipeline**  
  Полная автоматизация сборки под **amd64**, **arm64**, **armhf** (Ubuntu 24.04), с кэшированием `ccache`, упаковкой артефактов в `release/` и публикацией GitLab Release по тегу.
