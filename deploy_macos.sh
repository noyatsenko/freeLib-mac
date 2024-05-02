### Сборка и установка из исходников в MacOS

### Кросс-компиляции x86_64 на arm 
# softwareupdate --install-rosetta --agree-to-license
# arch -x86_64 zsh

##############################
# Настройка окружения сборки #
##############################

# HomeBrew
echo "Installing HomeBrew & CommandLineTools"
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

if [[ `uname -p` == arm ]]; then
    eval "$(/opt/homebrew/bin/brew shellenv)"
else
    eval "$(/usr/local/bin/brew shellenv)"
fi

# QT5
sleep 5
brew install cmake qt@5

PATH="$(brew --prefix)/opt/qt@5/bin:$PATH"
export LDFLAGS="-L$(brew --prefix)/opt/qt@5/lib"
export CPPFLAGS="-I$(brew --prefix)/opt/qt@5/include"

# По желанию настраиваем постоянное окружение QT5 (см. инструкцию в Терминале)
# echo 'export PATH="/opt/homebrew/opt/qt@5/bin:$PATH"' >> ~/.zshrc
# echo 'export LDFLAGS="-L/opt/homebrew/opt/qt@5/lib"' >> ~/.zshrc
# echo 'export CPPFLAGS="-I/opt/homebrew/opt/qt@5/include"' >> ~/.zshrc

##########
# Сборка #
##########

# Клонируем репозиторий проекта
cd ~
git clone --recurse-submodules https://github.com/noyatsenko/freeLib-mac.git freeLib

# Создаем директорию сборки и собираем
mkdir freeLib/build && cd freeLib/build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/Applications/freelib.app .. && cmake --build . -j4
mv /Applications/freelib.app /Applications/freelib_old.app
make install

# Собираем и подписываем релиз
macdeployqt /Applications/freelib.app
codesign --force --deep --sign - /Applications/freelib.app/

###########
# Очистка #
###########

# Удаляем временные файлы сборки
cd ../..
rm -rf ./freeLib


# Удаляем HomeBrew
read -q "REPLY?Remove HomeBrew and free 1Gb? (y/n) "
read -p "Remove HomeBrew and free 1Gb? (y/n)" -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/uninstall.sh)"
    echo "Clean HomeBrew data"
    sudo rm -r /opt/homebrew
    sudo rm -r /usr/local/homebrew
fi

# Удаляем CommandLineTools
read -q "REPLY?Remove CommandLineTools and free 4Gb? (y/n) "
read -p "Remove CommandLineTools and free 4Gb? (y/n)" -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    sudo rm -r /Library/Developer/CommandLineTools
fi
echo
