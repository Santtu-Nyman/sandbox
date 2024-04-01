import { Component, OnInit } from '@angular/core';
import { MenuController } from '@ionic/angular';

@Component({
  selector: 'app-pagenotfound',
  templateUrl: './pagenotfound.page.html',
  styleUrls: ['./pagenotfound.page.scss'],
})
export class PagenotfoundPage implements OnInit {

  constructor(public menu: MenuController) {
    this.menu.enable(false);
  }

  ngOnInit() {
  }

}
