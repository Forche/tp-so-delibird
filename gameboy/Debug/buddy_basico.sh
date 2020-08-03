#!/bin/sh
./gameboy BROKER CAUGHT_POKEMON 1 OK
sleep 2
./gameboy BROKER CAUGHT_POKEMON 2 FAIL
sleep 2
./gameboy BROKER NEW_POKEMON Pikachu 2 3 1
sleep 2
./gameboy BROKER CATCH_POKEMON Onyx 4 5
sleep 2
./gameboy SUSCRIPTOR NEW_POKEMON 10
sleep 2
./gameboy BROKER CATCH_POKEMON Charmander 4 5
sleep 2