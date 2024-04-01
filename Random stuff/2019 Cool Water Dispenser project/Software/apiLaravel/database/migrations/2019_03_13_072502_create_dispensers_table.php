<?php

use Illuminate\Support\Facades\Schema;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Database\Migrations\Migration;

class CreateDispensersTable extends Migration
{
    /**
     * Run the migrations.
     *
     * @return void
     */
    public function up()
    {
        Schema::create('dispensers', function (Blueprint $table) {
            $table->bigInteger('id');
            //$table->big('id');            //this is the serial number
            $table->unsignedInteger('area_id');
            $table->float('waterlevel')->nullable();
            $table->timestamp('lastChangedTime')->nullable();
            $table->float('temperature')->nullable();
            //$table->timestamp('lastCleanedTime');                 //om de een of andere reden krijg ik hier een foutmelding door
            $table->enum('mode', array('on', 'off'))->nullable()->default('on');
            $table->timestamps();

            $table->unique('id');
        });
    }

    /**
     * Reverse the migrations.
     *
     * @return void
     */
    public function down()
    {
        Schema::dropIfExists('dispensers');
    }
}
