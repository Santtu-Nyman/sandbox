import { Component, OnInit } from "@angular/core";
import {
  DatabaseService,
  IDispenser,
  ICompany,
  IBuilding,
  IFloor,
  IArea
} from "../../services/database.service";
import { NavController } from "@ionic/angular";

@Component({
  selector: "app-configure-company",
  templateUrl: "./configure-company.page.html",
  styleUrls: ["./configure-company.page.scss"]
})
export class ConfigureCompanyPage implements OnInit {
  allBuildings: IBuilding[];
  floors: IFloor[];
  areas: IArea[];
  dispensers: IDispenser[];
  selectedBuilding: IBuilding = null;
  selectedFloor: IFloor = null;
  selectedArea: IArea = null;
  selectedDispenser: IDispenser = null;
  isBuildingEditing: Boolean = false;
  isFloorEditing: Boolean = false;
  isAreaEditing: Boolean = false;
  isDispenserEditing: Boolean = false;
  newBuilding: IBuilding = {
    id: null,
    name: "",
    city: "",
    ZIPcode: null,
    address: "",
    company_id: null,
    created_at: "",
    updated_at: ""
  };
  newFloor: IFloor = {
    id: null,
    name: "",
    blueprint: "",
    building_id: null,
    created_at: "",
    updated_at: ""
  };
  newArea: IArea = {
    id: null,
    floor_id: null,
    name: "",
    conversionRate: null,
    startX: null,
    startY: null,
    endX: null,
    endY: null,
    created_at: "",
    updated_at: ""
  };
  newDispenser: IDispenser = {
    id: null,
    area_id: null,
    waterlevel: null,
    temperature: null,
    mode: null,
    lastChangedTime: "",
    lastCleanedTime: ""
  };
  isAddingBuilding: boolean = false;
  isAddingFloor: boolean = false;
  isAddingArea: boolean = false;
  isAddingDispenser: boolean = false;

  constructor(
    private dbService: DatabaseService,
    private router: NavController
  ) {}

  ngOnInit() {
    if (this.dbService.connectedCompany == null)
      this.router.navigateBack("connect-company");

    this.dbService.GetBuildingsOfCompany().subscribe(buildings => {
      this.allBuildings = buildings;
    });
  }

  navigateToDisconnectPage(){
    this.router.navigateBack("connect-company");
  }

  navigateToDisconnectPage(){
    this.router.navigateBack("connect-company");
  }

  get connectedCompanyName(): string {
    return this.dbService.connectedCompany.name;
  }

  GetCompanyFloors() {
    this.dbService
      .GetFloorsOfBuilding(this.selectedBuilding)
      .subscribe(floors => {
        this.floors = floors;
      });
  }

  GetCompanyAreas() {
    this.dbService.GetAreasOfFloor(this.selectedFloor).subscribe(areas => {
      this.areas = areas;
      console.log(this.areas);
    });
  }

  GetCompanyDispensers() {
    this.dbService
      .GetDispensersOfArea(this.selectedArea)
      .subscribe(dispensers => {
        this.dispensers = dispensers;
      });
  }

  selectBuilding = building => {
    if (building != this.selectedBuilding) {
      this.selectedBuilding = building;
      this.selectedFloor = null;
      this.selectedArea = null;
      this.selectedDispenser = null;
      this.GetCompanyFloors();
    }
  };

  selectFloor = floor => {
    if (floor != this.selectedFloor) {
      this.selectedFloor = floor;
      this.selectedArea = null;
      this.selectedDispenser = null;
      this.GetCompanyAreas();
    }
  };

  selectArea = area => {
    if (area != this.selectedArea) {
      this.selectedArea = area;
      this.selectedDispenser = null;
      this.GetCompanyDispensers();
    }
  };

  selectDispenser = dispenser => {
    if (dispenser != this.selectedDispenser) {
      this.selectedDispenser = dispenser;
    }
  };

  editBuilding = e => {
    this.isBuildingEditing = true;
  };
  addBuilding = e => {
    this.isAddingBuilding = true;
  };
  deleteBuilding = building => {
    this.dbService.DeleteBuilding(building).subscribe(buildings => {
      this.dbService.GetBuildingsOfCompany().subscribe(b => {
        this.allBuildings = b;
      });
      this.selectedBuilding = null;
      this.selectedFloor = null;
      this.selectedArea = null;
      this.selectDispenser = null;
      this.stopEditing();
    });
  };
  editFloor = e => {
    this.isFloorEditing = true;
  };
  addFloor = e => {
    this.isAddingFloor = true;
  };
  deleteFloor = floor => {
    this.dbService.DeleteFloor(floor).subscribe(floors => {
      this.GetCompanyFloors();
      this.selectedFloor = null;
      this.selectedArea = null;
      this.selectedDispenser = null;
      this.stopEditing();
    });
  };
  editArea = e => {
    this.isAreaEditing = true;
  };
  addArea = e => {
    this.isAddingArea = true;
  };
  deleteArea = area => {
    this.dbService.DeleteArea(area).subscribe(areas => {
      this.GetCompanyAreas();
      this.selectedArea = null;
      this.selectedDispenser = null;
      this.stopEditing();
    });
  };
  editDispenser = e => {
    this.isDispenserEditing = true;
  };
  addDispenser = e => {
    this.isAddingDispenser = true;
  };
  deleteDispenser = dispenser => {
    this.dbService.DeleteDispenser(dispenser).subscribe(dispensers => {
      this.GetCompanyDispensers();
      this.selectedDispenser = null;
      this.stopEditing();
    });
  };

  editCurrentBuilding = building => {
    //console.log(building);
    this.dbService.UpdateBuilding(building).subscribe(building => {
      this.dbService.GetBuildingsOfCompany().subscribe(buildings => {
        this.allBuildings = buildings;
        this.stopEditing();
      });
    });
  };

  addNewBuilding = building => {
    this.dbService.PostBuilding(building).subscribe(building => {
      this.dbService.GetBuildingsOfCompany().subscribe(buildings => {
        this.allBuildings = buildings;
        this.newBuilding = {
          id: null,
          name: "",
          city: "",
          ZIPcode: null,
          address: "",
          company_id: null,
          created_at: "",
          updated_at: ""
        };
        this.stopEditing();
      });
    });
  };

  editCurrentFloor = floor => {
    this.dbService
      .UpdateFloor(floor, this.selectedBuilding)
      .subscribe(floor => {
        this.stopEditing();
      });
  };

  addNewFloor = floor => {
    this.dbService
      .PostFloor(this.newFloor, this.selectedBuilding)
      .subscribe(floor => {
        this.GetCompanyFloors();
        this.newFloor = {
          id: null,
          name: "",
          blueprint: "",
          building_id: null,
          created_at: "",
          updated_at: ""
        };
        this.stopEditing();
      });
  };

  editCurrentArea = area => {
    this.dbService.UpdateArea(area, this.selectedFloor).subscribe(area => {
      this.stopEditing();
    });
  };

  addNewArea = area => {
    this.dbService.PostArea(area, this.selectedFloor).subscribe(area => {
      this.GetCompanyAreas();
      this.newArea = {
        id: null,
        floor_id: null,
        name: "",
        conversionRate: null,
        startX: null,
        startY: null,
        endX: null,
        endY: null,
        created_at: "",
        updated_at: ""
      };
      this.stopEditing();
    });
  };

  editCurrentDispenser = dispenser => {
    this.dbService
      .UpdateDispenser(dispenser, this.selectedArea)
      .subscribe(dispenser => {
        this.stopEditing();
      });
  };

  addNewDispenser = dispenser => {
    this.dbService
      .PostDispenser(dispenser, this.selectedArea)
      .subscribe(dispenser => {
        this.GetCompanyDispensers();
        this.newDispenser = {
          id: null,
          area_id: null,
          waterlevel: null,
          temperature: null,
          mode: null,
          lastChangedTime: "",
          lastCleanedTime: ""
        };
        this.stopEditing();
      });
  };

  stopEditing = () => {
    this.isBuildingEditing = false;
    this.isAddingBuilding = false;
    this.isFloorEditing = false;
    this.isAddingFloor = false;
    this.isAreaEditing = false;
    this.isAddingArea = false;
    this.isDispenserEditing = false;
    this.isAddingDispenser = false;
  };
}
