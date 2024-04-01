<?php

use Faker\Generator as Faker;

$factory->define(App\Area::class, function (Faker $faker) {
    return [
        'floor_id' => rand(1, 20),
        'name' => $faker->firstName,
        'conversionRate' => rand(0, 10)/10,
        'startX' => rand(0,10),
        'startY' => rand(0,10),
        'endX' => rand(0,10),
        'endY' => rand(0,10),
    ];
});
