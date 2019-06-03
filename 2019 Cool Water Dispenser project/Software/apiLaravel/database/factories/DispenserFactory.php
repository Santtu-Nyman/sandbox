<?php

use Faker\Generator as Faker;

$factory->define(App\Dispenser::class, function (Faker $faker) {
    return [
        'id' => 12345,
        'area_id' => rand(1, 40),
        'waterlevel' => rand(0,100)/100,
        'lastChangedTime' => $faker->dateTime,
        'temperature' => rand(0, 20),
        'mode' => 'on',
    ];
});
