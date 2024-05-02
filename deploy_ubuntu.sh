### Сборка и установка из исходников в Ubuntu
### Dev by https://github.com/petrovvlad/freeLib

# Установить необходимые компоненты:
sudo apt update && sudo apt-get -y install git cmake build-essential qtbase5-dev libqt5sql5-sqlite libquazip5-dev

#или в Ubuntu ≥ 22.04 можно установить Qt6:
if [[ `cat /etc/*release | grep RELEASE | awk -F'[=|.]' '{print $2}'` -ge 22 ]]; then
	sudo apt update && sudo apt-get -y install git cmake build-essential qt6-base-dev libqt6core5compat6-dev libqt6svg6 zlib1g-dev
fi

#Скачать исходники программы:
git clone --recurse-submodules https://github.com/petrovvlad/freeLib.git

#Собрать и установить:
mkdir freeLib/build && cd freeLib/build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr .. && cmake --build . -j2
sudo make install

##Удалить временные файлы сборки
cd ../..
rm -rf ./freeLib
