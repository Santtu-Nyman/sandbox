import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { Routes, RouterModule } from '@angular/router';

import { IonicModule } from '@ionic/angular';

import { FloorPage } from './floor.page';

const routes: Routes = [
  {
    path: '',
    component: FloorPage
  },
  {
    path: ':id',
    component: FloorPage
  }
];

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    IonicModule,
    RouterModule.forChild(routes)
  ],
  declarations: [FloorPage]
})
export class FloorPageModule {}
