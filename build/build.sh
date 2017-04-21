cd ../rapidlib/
rm build_debug -rf
mkdir build_debug
cd build_debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j 4

cd ../
rm build_release -rf
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j 4

cd ../../apploader/
rm build_debug -rf
mkdir build_debug
cd build_debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j 4

cd ../
rm build_release -rf
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j 4

cd ../../networkengine/
rm build_debug -rf
mkdir build_debug
cd build_debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j 4

cd ../
rm build_release -rf
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j 4

cd ../../
cp apploader/output/linux32d/apploader output/linux32d/
cp apploader/output/linux32/apploader output/linux32/

cp networkengine/output/linux32d/networkengine.so output/linux32d/
cp networkengine/output/linux32/networkengine.so output/linux32/

