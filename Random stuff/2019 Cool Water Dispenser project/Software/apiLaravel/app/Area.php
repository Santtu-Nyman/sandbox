<?php

namespace App;

use Illuminate\Database\Eloquent\Model;

class Area extends Model
{
    public function dispensers(){
        return $this->hasMany(Dispenser::class);
    }
}
