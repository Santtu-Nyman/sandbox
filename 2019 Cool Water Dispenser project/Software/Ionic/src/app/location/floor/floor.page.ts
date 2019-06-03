import { Component, OnInit, ViewChild, ElementRef } from '@angular/core';
import { ActivatedRoute, Router } from '@angular/router';
import { NavController } from '@ionic/angular';
import { DatabaseService, IFloor, IArea, IDispenser, IBuilding} from '../../services/database.service';
import { HttpErrorResponse } from '@angular/common/http';

@Component({
  selector: 'app-floor',
  templateUrl: './floor.page.html',
  styleUrls: ['./floor.page.scss'],
})
export class FloorPage implements OnInit {
  Markerwidth: number;
  Markerheight: number;
  MarkerXpos: number;
  MarkerYpos: number;

  markerArray = new Array();

  mouseXpos: number;
  mouseYpos: number;

  CanvWidth = 626;
  canvHeight = 434;

  mapSprite = new Image();
  Sprite = new Image();


  id: any;

  floors: IFloor;
  areas: IArea;
  Dispenser: IArea;
  photo: IFloor;
  Dispensers: IDispenser;
  constructor (public data: DatabaseService, public activatedRoute: ActivatedRoute, public navCtrl: NavController, public route: Router, public service: DatabaseService) { 
    this.id = this.activatedRoute.snapshot.params['id'];
    this.data.GetSingleFloor(this.id).subscribe((floor) => {
      console.log(floor);
      this.floors = floor;
      this.photo = floor;
    },
    (error: HttpErrorResponse) => {
       this.route.navigate(['/pagenotfound']);
    });

    this.data.GetArea().subscribe((area) => {
      console.log(area);
      this.areas = area;
    });

    this.data.GetSingleArea(this.id).subscribe((singleArea) => {
      console.log(singleArea);
      if (singleArea.id == this.id) {
        console.log('floor id:', singleArea.floor_id);
      }
      this.Dispenser = singleArea;
      console.log('x 1ste: ' + this.Dispenser.startX);
      console.log('y 1ste: ' + this.Dispenser.startY);
      console.log('x 2de: ' + this.Dispenser.endX);
      console.log('y 2de: ' + this.Dispenser.endY);
      this.draw(this.floors.blueprint, this.Dispenser.startX, this.Dispenser.endY, this.Dispenser.endX, this.Dispenser.startY); 
    });


    this.data.GetSingleDispensers(this.id).subscribe((dispens) =>{
      this.Dispensers = dispens;
      if (this.Dispenser.floor_id == this.id) {
        console.log('name: ' + this.Dispenser.name);
        console.log('id ' + this.Dispensers.id);
        console.log('area_id ' + this.Dispensers.area_id);
        console.log('waterlevel ' + this.Dispensers.waterlevel);
      }
    });
  }

  ionViewDidEnter(){
    var interval = setInterval(()=> {this.refreshPage() },500); 
    var cancel = setInterval(()=> {clearInterval(interval); clearInterval(cancel) },3000); 
    
    this.draw(this.floors.blueprint, this.Dispenser.startX, this.Dispenser.endY, this.Dispenser.endX, this.Dispenser.startY); 
  }
  
  refreshPage() {
    this.draw(this.floors.blueprint, this.Dispenser.startX, this.Dispenser.endY, this.Dispenser.endX, this.Dispenser.startY); 
  }

  ngOnInit() {

  }

  Marker() {
    this.Markerwidth = 12;
    this.Markerheight = 20;
    this.MarkerXpos = 0;
    this.MarkerYpos = 0;
  }

  //#region Draw
  draw(image: string, startX: number, endY: number, endX: number, startY: number) {
/*     this.canvas = <HTMLCanvasElement> document.getElementById('Canvas')
    this.context = this.canvas.getContext('2d')!; */

    const canvas: HTMLCanvasElement = <HTMLCanvasElement> document.getElementById('Canvas');
    const context: CanvasRenderingContext2D = canvas.getContext('2d');

    canvas.width = this.CanvWidth;
    canvas.height = this.canvHeight;

    context.fillStyle = '#000';
    context.fillRect(0, 0, canvas.width, canvas.height);

    this.mapSprite.src = image;
    this.Sprite.src = 'https://www.lasvegas-waterdelivery.com/wp-content/uploads/2016/07/5gal-cropped.png'

    context.drawImage(this.mapSprite, 0, 0, this.CanvWidth, this.canvHeight);
    context.drawImage(this.Sprite, startX, startY, 12, 20);

    context.stroke();
  }
  //#endregion
}
