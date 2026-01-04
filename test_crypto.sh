#!/bin/bash

API="http://localhost:8080"

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

header() {
    echo -e "${GREEN}\n!!! $1 !!!${NC}"
}

pause() {
    echo -e "${YELLOW}\n[ПАУЗА] ${NC}"
    read
}

call() {
    local method=$1
    local path=$2
    local key=$3
    local data=$4

    if [ "$method" == "GET" ]; then
        curl -s -X GET "$API$path" -H "X-USER-KEY: $key"
    elif [ "$method" == "POST" ]; then
        curl -s -X POST "$API$path" -H "X-USER-KEY: $key" -d "$data"
    elif [ "$method" == "DELETE" ]; then
        curl -s -X DELETE "$API$path" -H "X-USER-KEY: $key" -d "$data"
    fi
}

header "1 Лоты и пары"
call "GET" "/lot"
call "GET" "/pair"

header "2 Регистрация пользователей"
KEY1=$(call "POST" "/user" "" '{"username": "ivan"}' | grep -oE '[a-f0-9]{32}')
KEY2=$(call "POST" "/user" "" '{"username": "petr"}' | grep -oE '[a-f0-9]{32}')
echo "Ключ Ивана: $KEY1"
echo "Ключ Петра: $KEY2"

pause 

header "3 Иван создает орден"
echo "Иван покупает 100 RUB за BTC (тратит 100 * 0.01 = 1 BTC)"
call "POST" "/order" "$KEY1" '{"pair_id": 1, "quantity": 100, "price": 0.01, "type": "buy"}'

pause 

header "4 Петр выставляет встречный ордер и происходит мэтч"
echo "Петр продает 100 RUB за 1 BTC"
call "POST" "/order" "$KEY2" '{"pair_id": 1, "quantity": 100, "price": 0.01, "type": "sell"}'

pause 

header "5 Тест удаления ордера"
echo "Иван создает ордер для удаления"
call "POST" "/order" "$KEY1" '{"pair_id": 2, "quantity": 50, "price": 10, "type": "buy"}'

ORDER_ID=$(call "GET" "/order" | grep -oE '"order_id": [0-9]+' | tail -n 1 | awk '{print $2}')
echo " Создан ордер ID: $ORDER_ID. Сейчас он заблокировал средства"

pause 

echo "Иван передумал, удаляем ордер $ORDER_ID..."
call "DELETE" "/order" "$KEY1" "{\"order_id\": $ORDER_ID}"

header "6 Возрат средств"
echo "Баланс Ивана после удаления ордера:"
call "GET" "/balance" "$KEY1"

echo -e "${GREEN}\n Тест завершен.${NC}"
