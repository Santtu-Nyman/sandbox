<?php

namespace App;

use Illuminate\Database\Eloquent\Model;

class Floor extends Model
{
    public function areas(){
        return $this->hasMany(Area::class);
    }
}
