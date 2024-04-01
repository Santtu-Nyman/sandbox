import { Component } from '@angular/core';
import { Router } from '@angular/router';

@Component({
  selector: 'app-home',
  templateUrl: 'home.page.html',
  styleUrls: ['home.page.scss'],
})
export class HomePage {

  constructor(public route: Router) { }

  navigateToLocation() {
    this.route.navigate(['location/']);
  }

  navigateToConfiguration() {
    this.route.navigate(['configure-company/']);
  }
}
