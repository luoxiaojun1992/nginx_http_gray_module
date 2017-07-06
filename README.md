# nginx_http_gray_module

## Features
+ Nginx Virtual Host A/B Testing

## Installation
+ git clone git@github.com:luoxiaojun1992/nginx_http_gray_module.git
+ ./configure --add-module=YOUR_DIR/nginx_http_gray_module

## Configuration
+ edit nginx.conf
```
gray test_redis_key
```

## Usage
+ set redis
```
redis-cli
set test_redis_key_switch on
set test_redis_key_gray_env beta
```
+ edit nginx.conf
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

+ curl request
curl -v -H "X-API-ENV: beta" http://127.0.0.1/xxx
