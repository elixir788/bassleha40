# Басс от лехи40 (VST3)

Плагин для обработки басса: делает его плотнее, глубже и качественнее.
Интерфейс в стиле Fruity Aero (стекло, вода, пузырьки). Он меняется вместе с крутилками.

## Как собрать (Windows)

Нужно: Visual Studio 2022 (с C++), CMake 3.22+, интернет (JUCE скачается сам).

```
cd BassOtLehi40
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

После сборки `Obrabotka Bassa ot Lehi40.vst3` копируется в
`C:\Program Files\Common Files\VST3` (если не получилось, запусти консоль от администратора
или скопируй файл из `build\BassOtLehi40_artefacts\Release\VST3` вручную).
Потом в FL Studio / Ableton нажми «Rescan plugins».

## Крутилки

Граница басса, Глубина, Плотность, Гармоники, Сжатие, Чистка низа, Моно-бас, Микс, Громкость.
Рядом с каждой есть стрелка: наведи на неё курсор, чтобы прочитать, что делает крутилка
(клик по стрелке закрепляет подсказку).

## Сборка без установки программ (через GitHub)

1. На Mac в Finder нажми Cmd+Shift+. (точка), чтобы показать скрытую папку .github.
2. Зарегистрируйся на github.com, создай новый репозиторий (New repository).
3. Нажми «uploading an existing file» и перетащи ВСЁ содержимое папки
   (.github, Source, CMakeLists.txt, README.md). Нажми Commit.
4. Вкладка Actions: дождись зелёных галочек (10–20 минут).
5. Открой запуск, внизу в Artifacts скачай Bass-ot-Lehi40-Mac и распакуй.
6. Скопируй «Obrabotka Bassa ot Lehi40.vst3» в
   /Library/Audio/Plug-Ins/VST3  (или ~/Library/Audio/Plug-Ins/VST3).
7. Терминал: xattr -cr "/Library/Audio/Plug-Ins/VST3/Obrabotka Bassa ot Lehi40.vst3"
8. FL Studio: Options > Manage plugins > Find installed plugins.
