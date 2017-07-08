# nginx_http_gray_module

## Features
+ Nginx Virtual Host A/B Testing

## Installation
+ Clone project
```
git clone git@github.com:luoxiaojun1992/nginx_http_gray_module.git
```

+ Make nginx
```
./configure --add-module=YOUR_DIR/nginx_http_gray_module
```

## Configuration
+ Edit nginx.conf
```
gray test_redis_key
```

## Usage
+ Set redis
```
redis-cli
set test_redis_key_switch on
set test_redis_key_gray_env beta
```

+ Edit nginx.conf
```
#If Need A/B Testing
if ($is_gray) {
  ...
}

#If Not Need A/B Testing
if ($is_not_gray) {
  ...
}
```

+ Curl request
```
curl -v -H "X-API-ENV: beta" http://127.0.0.1/xxx
```
