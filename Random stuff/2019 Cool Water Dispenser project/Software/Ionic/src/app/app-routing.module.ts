import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

const routes: Routes = [
  {
    path: '',
    redirectTo: 'home',
    pathMatch: 'full'
  },
  {
    path: 'home',
    loadChildren: './home/home.module#HomePageModule'
  },
  {
    path: 'location',
    loadChildren: './location/location.module#LocationPageModule'
  },
  {
    path: `location/:name`,
    loadChildren: './location/buildings/buildings.module#BuildingsPageModule'
  },
  {
    path: 'location/:name/floor/:id',
    loadChildren: './location/floor/floor.module#FloorPageModule'
  },
  {
    path: 'connect-company',
    loadChildren: './CompanyConfiguration/connect-company/connect-company.module#ConnectCompanyPageModule' 
  },
  {
    path: 'configure-company',
    loadChildren: './CompanyConfiguration/configure-company/configure-company.module#ConfigureCompanyPageModule'
  },
  {
    path: 'pagenotfound',
    loadChildren: './pagenotfound/pagenotfound.module#PagenotfoundPageModule'
  }
];

@NgModule({
  imports: [RouterModule.forRoot(routes)],
  exports: [RouterModule]
})
export class AppRoutingModule {}
