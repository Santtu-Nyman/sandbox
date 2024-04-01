import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { Routes, RouterModule } from '@angular/router';
import { IonicModule } from '@ionic/angular';
import { BuildingsPage } from './buildings.page';

const routes: Routes = [
  {
    path: '',
    component: BuildingsPage
  }, 
  {
    path: `:buildingName`,
    component: BuildingsPage
  }

];

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    IonicModule,
    RouterModule.forChild(routes)
  ],
  declarations: [BuildingsPage]
})
export class BuildingsPageModule {}
