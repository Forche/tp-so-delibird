#!/bin/sh
./gameboy BROKER CAUGHT_POKEMON 1 OK
sleep 1
./gameboy BROKER CAUGHT_POKEMON 2 FAIL
sleep 1

./gameboy BROKER CATCH_POKEMON Pikachu 2 3
sleep 1
./gameboy BROKER CATCH_POKEMON Squirtle 5 2
sleep 1

./gameboy BROKER CATCH_POKEMON Onyx 4 5
sleep 1

./gameboy SUSCRIPTOR CAUGHT_POKEMON 10

./gameboy BROKER CATCH_POKEMON Vaporeon 4 5
sleep 1