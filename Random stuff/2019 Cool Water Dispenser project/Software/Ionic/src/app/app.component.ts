import { Component } from '@angular/core';

import { Platform } from '@ionic/angular';
import { SplashScreen } from '@ionic-native/splash-screen/ngx';
import { StatusBar } from '@ionic-native/status-bar/ngx';
import { HomePage } from './home/home.page';

@Component({
  selector: 'app-root',
  templateUrl: 'app.component.html'
})
export class AppComponent {
  rootPage: any = HomePage;
  public appPages = [
    {
      title: 'Home',
      url: '/home',
      icon: 'home'
    },
    {
      title: 'Locations',
      url: '/location',
      icon: 'map'
    },
    {
      title: 'Configuration Panel',
      url: '/configure-company',
      icon: 'settings'
    }
  ];

  constructor(
    private platform: Platform,
    private splashScreen: SplashScreen,
    private statusBar: StatusBar
  ) {
    this.initializeApp();
  }

  initializeApp() {
    this.platform.ready().then(() => {
      this.statusBar.styleBlackTranslucent();
      this.splashScreen.hide();

      if (this.platform.is('desktop') || this.platform.is('mobileweb')) {
        console.log('Platform is core or is mobile web');
      } else {
        let notificationOpenedCallback = function (jsonData) {
          console.log('notificationOpenedCallback: ' + JSON.stringify(jsonData));
        };

        window["plugins"].OneSignal
          .startInit("24f183e8-c918-4b4a-9464-a4f17c9c08c2", "429763097094")
          .handleNotificationOpened(notificationOpenedCallback)
          .endInit();
      }
    });
  }
}
