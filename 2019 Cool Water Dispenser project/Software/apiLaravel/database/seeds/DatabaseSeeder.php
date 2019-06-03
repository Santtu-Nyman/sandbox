<?php

use Illuminate\Database\Seeder;

class DatabaseSeeder extends Seeder
{
    /**
     * Seed the application's database.
     *
     * @return void
     */
    public function run()
    {
        factory('App\Company', 5)->create();
        factory('App\Building', 7)->create();
        factory('App\Floor', 20)->create();
        factory('App\Area', 40)->create();
        factory('App\Dispenser', 1)->create();
    }
}
