<?php

use Faker\Generator as Faker;

$factory->define(App\Floor::class, function (Faker $faker) {
    return [
        'building_id' => rand(1, 7),
        'name' => $faker->firstName,
        'blueprint' => str_random(10),
    ];
});
