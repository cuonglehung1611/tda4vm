## How to build
cd ~/soa_demo
g++ -std=c++17 -O2 -Wall -Wextra provider.cpp -o provider
g++ -std=c++17 -O2 -Wall -Wextra consumer.cpp -o consumer

## How to run
Terminal 1:
```
cd ~/soa_demo
./provider
```

Terminal 2:
```
cd ~/soa_demo
./consumer
```