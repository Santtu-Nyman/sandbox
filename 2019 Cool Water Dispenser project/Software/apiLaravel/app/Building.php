<?php

namespace App;

use Illuminate\Database\Eloquent\Model;

class Building extends Model
{
    public function floors(){
        return $this->hasMany(Floor::class);
    }
}
