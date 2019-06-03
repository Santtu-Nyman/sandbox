import { Component, OnInit } from '@angular/core';
import { NavController} from '@ionic/angular';
import { ActivatedRoute, Router } from '@angular/router';
import { DatabaseService, IBuilding, ICompany, IFloor } from '../../services/database.service';
import { HttpErrorResponse } from '@angular/common/http';
@Component({
  selector: 'app-buildings',
  templateUrl: './buildings.page.html',
  styleUrls: ['./buildings.page.scss'],
})
export class BuildingsPage implements OnInit {

  info = null;
  id: any;

  companies: ICompany;
  building: IBuilding;
  floors: IFloor;

  nameComp : string = "";
  constructor (public route: Router, public data: DatabaseService, private activatedRoute: ActivatedRoute, public navCtrl: NavController) {
    this.id = this.activatedRoute.snapshot.params['name'];
    this.data.GetSingleBuilding(this.id).subscribe((building) => {
      console.log(building);
      this.building = building;
    },
    (error: HttpErrorResponse) => {
       this.route.navigate(['/pagenotfound']);
    });

    this.data.GetCompany(this.nameComp).subscribe((company) => {
        this.companies = company;
    });

    this.data.GetFloor().subscribe((floor) => {
      this.floors = floor;
    });
  }

  ngOnInit() {

  }

  navigateToChosenFloor(id: string){
    this.route.navigate([`location/${this.id}/floor/` + id]); 
  }

  get ImageUrl() {
    return 'http://www.dagvandearchitectuur.be/2015/website/_images/_thumbs/360h/APellerman_antwerpen_stramienstramien_F4.jpg'
  }
}
