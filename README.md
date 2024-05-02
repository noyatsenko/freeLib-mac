# freeLib
freeLib для MacOS - каталогизатор для библиотек LibRusEc и Flibusta

Это форк общедоступного freeLib 5.0 , разработка которого прекращена. 
![screenshot](./freeLib/doc/screenshot.png#gh-light-mode-only)
![screenshot](./freeLib/doc/screenshot-dark.png#gh-dark-mode-only)
* Создание собственных библиотек на основе файлов FB2(.ZIP), EPUB, FBD.
* Конвертация в форматы AZW3 (KF8), MOBI, MOBI7 (KF7), EPUB.
* Работа с несколькими библиотеками.
* Импорт библиотек из inpx-файлов.
* Поиск и фильтрация книг.
* Серверы OPDS и HTTP.
* Сохранение книг в выбранную папку.
* Различные настройки экспорта для нескольких устройств.
* Отправка выбранных файлов книг на email (Send to Kindle).
* Установка тегов для книги, автора, серии и фильтрация по тегам.
* Настройка форматирования книг (шрифты, буквица, заголовки, переносы, сноски)
* Чтение книг с помощью внешних приложений. Можно назначить отдельную программу для каждого формата.


### Сборка и установка из исходников в MacOS


Установить и настроить HomeBrew 
[https://brew.sh](https://brew.sh)

Установить компоненты QT
```
brew install cmake qt6
```

Скачать и собрать проект
```
git clone --recurse-submodules https://github.com/noyatsenko/freeLib-mac.git freeLib
mkdir freeLib/build && cd freeLib/build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/Applications/freelib.app .. && cmake --build . -j4
mv /Applications/freelib.app /Applications/freelib_old.app
make install
```

Удалить временные файлы сборки
```
cd ../..
rm -rf ./freeLib
```

Можно удалить CommandLineTools (4Гб)
```
sudo rm -r /Library/Developer/CommandLineTools
```

HomeBrew не удалять
