<?php

/*
|--------------------------------------------------------------------------
| Web Routes
|--------------------------------------------------------------------------
|
| Here is where you can register web routes for your application. These
| routes are loaded by the RouteServiceProvider within a group which
| contains the "web" middleware group. Now create something great!
|
*/

Route::get('/', function () {
    return view('welcome');
 });

Route::resource('/api/company', 'CompaniesController');
Route::resource('/api/building', 'BuildingsController');
Route::resource('/api/technician', 'TechniciansController');
Route::resource('/api/floor', 'FloorsController');
Route::resource('/api/area', 'AreasController');
Route::resource('/api/dispenser', 'DispensersController');
Route::resource('/api/history', 'HistoriesController');