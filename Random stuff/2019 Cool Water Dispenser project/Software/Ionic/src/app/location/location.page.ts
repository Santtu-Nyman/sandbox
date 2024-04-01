import { Component, OnInit } from '@angular/core';
import { NavController} from '@ionic/angular';
import { DatabaseService, IBuilding } from '../../app/services/database.service';
import { Observable } from 'rxjs';
import { Router } from '@angular/router';

@Component({
  selector: 'app-location',
  templateUrl: './location.page.html',
  styleUrls: ['./location.page.scss'],
})

export class LocationPage implements OnInit {

  Buildings: IBuilding[];
  results: Observable<any>;
  constructor(public route: Router, public navCrtl: NavController, public data: DatabaseService) {
    this.data.getBuildings().subscribe((building) => {
      if (building != null) {
        this.Buildings = building;
      } else {
        this.route.navigate(['/pagenotfound']);
        console.log(building);
      }
    });
  }

  ngOnInit() {

  }

  navigateToChosenBuilding(id: string) {
    this.route.navigate([`location/` + id]);
  }
}
