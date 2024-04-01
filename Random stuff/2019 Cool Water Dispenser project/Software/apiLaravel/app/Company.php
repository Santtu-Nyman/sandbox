<?php

namespace App;

use Illuminate\Database\Eloquent\Model;

class Company extends Model
{
    public function buildings()
    {
        return  $this->hasMany(Building::class);
        //return $this->hasMany('App\Building');
    }
}
