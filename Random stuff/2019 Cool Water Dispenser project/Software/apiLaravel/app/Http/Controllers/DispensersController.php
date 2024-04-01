<?php

namespace App\Http\Controllers;

use App\Dispenser;
use App\Area;
use Illuminate\Http\Request;
use OneSignal;

class DispensersController extends Controller
{
    /**
     * Display a listing of the resource.
     *
     * @return \Illuminate\Http\Response
     */
    public function index(Request $req)
    {
        if ($req->filled('area_id'))
            return Area::find(request('area_id'))->dispensers;
        else
            return Dispenser::all();
    }

    /**
     * Show the form for creating a new resource.
     *
     * @return \Illuminate\Http\Response
     */
    public function create()
    {
        //
    }

    /**
     * Store a newly created resource in storage.
     *
     * @param  \Illuminate\Http\Request  $request
     * @return \Illuminate\Http\Response
     */
    public function store(Request $request)
    {
        // Dispenser::create([
        //     'waterlevel' => request('waterlevel'),
        //     'lastChangedTime' => request('lastChangedTime'),
        //     'temperature' => request('temperature'),
        //     'mode' => request('mode'),
        //     'areaID' => request('areaID')
        // ]);

        $dispenser = new Dispenser();
        $dispenser->id= $request->input('id');
        $dispenser->area_id = $request->input('area_id');
        $dispenser->waterlevel = $request->input('waterlevel');
        $dispenser->lastChangedTime = $request->input('lastChangedTime');
        $dispenser->temperature = $request->input('temperature');
        $dispenser->mode = $request->input('mode');

        $dispenser->save();

        return Dispenser::find($request->input('id'));
    }

    /**
     * Display the specified resource.
     *
     * @param  \App\Dispenser  $dispenser
     * @return \Illuminate\Http\Response
     */
    public function show(Dispenser $dispenser)
    {
        return $dispenser;
    }

    /**
     * Show the form for editing the specified resource.
     *
     * @param  \App\Dispenser  $dispenser
     * @return \Illuminate\Http\Response
     */
    public function edit(Dispenser $dispenser)
    {
        //
    }

    /**
     * Update the specified resource in storage.
     *
     * @param  \Illuminate\Http\Request  $request
     * @param  \App\Dispenser  $dispenser
     * @return \Illuminate\Http\Response
     */
    public function update(Request $request, Dispenser $dispenser)
    {
        if (request("waterlevel") <= 0.20 && $dispenser->waterlevel > 0.20){
            $value = request("waterlevel") * 100;
            OneSignal::sendNotificationToAll(
                "Dispencer {$dispenser->id} only has {$value}% water left!", 
                $url = null, 
                $data = null, 
                $buttons = null, 
                $schedule = null
            );
        }

        if ($request->filled('area_id')){
            $dispenser->area_id = $request->input('area_id');
        }
        if ($request->filled('waterlevel')){
            $dispenser->waterlevel = $request->input('waterlevel');
        }
        if ($request->filled('lastChangedTime')){
            $dispenser->lastChangedTime = $request->input('lastChangedTime');
        }
        if($request->filled('temperature')){
            $dispenser->temperature = $request->input('temperature');
        }
        if($request->filled('mode')){
            $dispenser->mode = $request->input('mode');
        }
        
        $dispenser->save();
        return $dispenser;
    }

    /**
     * Remove the specified resource from storage.
     *
     * @param  \App\Dispenser  $dispenser
     * @return \Illuminate\Http\Response
     */
    public function destroy(Dispenser $dispenser)
    {
        $dispenser->delete();
        return Dispenser::all();
    }
}
