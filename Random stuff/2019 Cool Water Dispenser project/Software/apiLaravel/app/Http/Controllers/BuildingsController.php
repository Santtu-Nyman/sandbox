<?php

namespace App\Http\Controllers;

use App\Building;
use App\Company;
use Illuminate\Http\Request;

class BuildingsController extends Controller
{
    /**
     * Display a listing of the resource.
     *
     * @return \Illuminate\Http\Response
     */
    public function index(Request $req)
    {
        if ($req->filled('company_id')) {
            $companybuildings = Company::find(request('company_id'))->buildings;

            return $companybuildings;
        } else
            return Building::all();
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
        $building = new Building();

        $building->name = request('name');
        $building->city = request('city');
        $building->ZIPcode = request('ZIPcode');
        $building->address = request('address');
        $building->company_id = request('company_id');

        $building->save();

        return $building;
    }

    /**
     * Display the specified resource.
     *
     * @param  \App\Building  $building
     * @return \Illuminate\Http\Response
     */
    public function show(Building $building)
    {
        return $building;
    }

    /**
     * Show the form for editing the specified resource.
     *
     * @param  \App\Building  $building
     * @return \Illuminate\Http\Response
     */
    public function edit(Building $building)
    {
        //
    }

    /**
     * Update the specified resource in storage.
     *
     * @param  \Illuminate\Http\Request  $request
     * @param  \App\Building  $building
     * @return \Illuminate\Http\Response
     */
    public function update(Request $request, Building $building)
    {
        $building->name = $request->input('name');
        $building->city = $request->input('city');
        $building->ZIPcode = $request->input('ZIPcode');
        $building->address = $request->input('address');
        $building->company_id = $request->input('company_id');

        $building->save();
        return $building;
    }

    /**
     * Remove the specified resource from storage.
     *
     * @param  \App\Building  $building
     * @return \Illuminate\Http\Response
     */
    public function destroy(Building $building)
    {
        $building->delete();
        return Building::all();
        //return redirect()->action("BuildingsController@index");
    }
}
