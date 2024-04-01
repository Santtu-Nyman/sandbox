<?php

use App\Building;
use Illuminate\Support\Str;
use Faker\Generator as Faker;

$factory->define(App\Building::class, function (Faker $faker) {
    return [
        'company_id' => rand(1, 5),         //in the databaseSeeder 5 companies are created
        'name' => $faker->name,
        'city' => $faker->city,
        'ZIPcode' => rand(1000, 4000),
        'address' => $faker->streetName,
    ];
});
