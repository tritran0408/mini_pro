# mini_pro
This repo is used for practicing mini projects 
## Enviroment
### GitPod

[![Gitpod](https://gitpod.io/button/open-in-gitpod.svg)](https://gitpod.io/#https://github.com/tritran0408/mini_pro)

Test on prebuilt enviroment with GitPod. More infomation about [Gitpod](https://gitpod.io/)

### Test on your linux machine
Requirement libs:
- jasson
- libjwt
- libcurl
- libmicrohttpd
- cmake
- autotools-dev

```bash
chmod +x ./libs.sh
./libs.sh
```
## Build
```bash
mkdir build 
cd build
cmake ..
make
```
## Command

Start server:
```bash
./Server -k ../keys/rsa_key_2048-pub.pem -p 8000 -c sub=user0
```

Start client:
```bash
./Client -t ../user0.jwt -t ../user1.jwt -p 8000
```
press Enter to exit 
