<?php

namespace App\Http\Controllers;

use App\Area;
use App\Floor;
use Illuminate\Http\Request;

class AreasController extends Controller
{
    /**
     * Display a listing of the resource.
     *
     * @return \Illuminate\Http\Response
     */
    public function index(Request $req)
    {
        if ($req->filled('floor_id'))
            return Floor::find(request('floor_id'))->areas;
        else
            return Area::all();
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
        $area = new Area();

        $area->name = request('name');
        $area->conversionRate = request('conversionRate');
        $area->startX = request('startX');
        $area->startY = request('startY');
        $area->endX = request('endX');
        $area->endY = request('endY');
        $area->floor_id = request('floor_id');

        $area->save();

        return $area;
    }

    /**
     * Display the specified resource.
     *
     * @param  \App\Area  $area
     * @return \Illuminate\Http\Response
     */
    public function show(Area $area)
    {
        return $area;
    }

    /**
     * Show the form for editing the specified resource.
     *
     * @param  \App\Area  $area
     * @return \Illuminate\Http\Response
     */
    public function edit(Area $area)
    {
        //
    }

    /**
     * Update the specified resource in storage.
     *
     * @param  \Illuminate\Http\Request  $request
     * @param  \App\Area  $area
     * @return \Illuminate\Http\Response
     */
    public function update(Request $request, Area $area)
    {
        $area->name = $request->input('name');
        if ($request->filled('conversionRate')){
            $area->conversionRate = $request->input('conversionRate');
        }
        if ($request->filled('startX')){
            $area->startX = $request->input('startX');
        }
        if ($request->filled('startY')){
            $area->startY = $request->input('startY');
        }
        if ($request->filled('endX')){
            $area->endX = $request->input('endX');
        }
        if ($request->filled('endY')){
            $area->endY = $request->input('endY');
        }
        $area->floor_id = $request->input('floor_id');

        $area->save();
        return $area;
    }

    /**
     * Remove the specified resource from storage.
     *
     * @param  \App\Area  $area
     * @return \Illuminate\Http\Response
     */
    public function destroy(Area $area)
    {
        $area->delete();
        return Area::all();
    }
}
