# HomeBrew
echo "Installing HomeBrew & CommandLineTools"
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

eval "$(/opt/homebrew/bin/brew shellenv)"

brew install cmake qt

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
# because @rpath fails
mv /Applications/freelib.app /opt/homebrew/freelib.app
macdeployqt /opt/homebrew/freelib.app
mv /opt/homebrew/freelib.app /Applications/freelib.app

codesign --force --deep --sign - /Applications/freelib.app/

###########
# Очистка #
###########

# Удаляем временные файлы сборки
cd ~
rm -rf ./freeLib


# Удаляем HomeBrew
read -q "REPLY?Remove HomeBrew and free 1Gb? (y/n) "
read -p "Remove HomeBrew and free 1Gb? (y/n)" -n 1 -r
if [[ $REPLY =~ ^[Yy]$ ]]; then
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/uninstall.sh)"
    echo "Clean HomeBrew data"
    sudo rm -r /opt/homebrew
fi

# Удаляем CommandLineTools
read -q "REPLY?Remove CommandLineTools and free 4Gb? (y/n) "
read -p "Remove CommandLineTools and free 4Gb? (y/n)" -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    sudo rm -r /Library/Developer/CommandLineTools
fi
echo
