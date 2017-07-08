# nginx_http_gray_module

## Features
+ Nginx Virtual Host A/B Testing

## Dependencies
+ Nginx
+ Redis

## Installation
+ Clone project
```
git clone git@github.com:luoxiaojun1992/nginx_http_gray_module.git
```

+ Compile Nginx
```
./configure --add-module=YOUR_DIR/nginx_http_gray_module
```

## Configuration
+ Edit Nginx configuration
```
gray test_redis_key 127.0.0.1 6379
```

## Usage
+ Set redis
```
redis-cli
set test_redis_key_switch on
set test_redis_key_gray_env beta
```

+ Edit Nginx configuration
```
if ($is_gray) {
  ...
}
if ($is_not_gray) {
  ...
}
```

+ Curl request
```
curl -v -H "X-API-ENV: beta" http://127.0.0.1/xxx
```
