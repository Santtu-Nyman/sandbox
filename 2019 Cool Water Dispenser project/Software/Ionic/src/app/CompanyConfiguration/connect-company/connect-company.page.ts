import { Component, OnInit } from "@angular/core";
import { NavController, ToastController } from "@ionic/angular";
import { DatabaseService, ICompany } from "../../services/database.service";

@Component({
  selector: "app-connect-company",
  templateUrl: "./connect-company.page.html",
  styleUrls: ["./connect-company.page.scss"]
})
export class ConnectCompanyPage implements OnInit {
  connectedToCompany: boolean = false;
  connectCompanyName: string = "";

  constructor(
    private dbService: DatabaseService,
    private router: NavController,
    private toastMessage: ToastController
  ) {}

  ngOnInit() {
    if (this.dbService.connectedCompany == null)
      console.log("not connected to a company");
    if (this.dbService.connectedCompany != null) {
      console.log(this.dbService.connectedCompany.name);
      this.connectedToCompany = true;
    }
  }

  public get Company(): ICompany {
    return this.dbService.connectedCompany;
  }

  MakeCompany(): void {
    this.dbService.GetCompany(this.connectCompanyName).subscribe(
      res => {
        this.showToast("company already exists");
        this.connectCompanyName = "";
      },
      error => {
        this.dbService.PostCompany(this.connectCompanyName).subscribe(
          res => {
            console.log(res);
            this.Connect(res);
            this.showToast("company succesfuly made and connected to");
          },
          error => {
            console.log(error);
          }
        );
      }
    );
  }

  SearchCompany(): void {
    this.dbService.GetCompany(this.connectCompanyName).subscribe(
      res => {
        console.log(res);
        this.Connect(res);
      },
      error => {
        console.log(error);
        this.showToast("company is not known");
      }
    );
  }

  Connect(comp: ICompany): void {
    this.dbService.connectedCompany = comp;
    this.connectedToCompany = true;
  }

  Disconnect(): void {
    this.dbService.connectedCompany = null;
    this.connectedToCompany = false;
    this.connectCompanyName = "";
  }

  GoToNextPage(): void {
    this.router.navigateForward("configure-company");
  }

  async showToast(message: string) {
    const toast = await this.toastMessage.create({
      message: message,
      duration: 2000
    });
    toast.present();
  }
}
