import { Injectable } from "@angular/core";
import {
  HttpClient,
  HttpErrorResponse,
  HttpParams
} from "@angular/common/http";

import { Observable, throwError } from "rxjs";
import { map, catchError } from "rxjs/operators";
import { Config } from "@ionic/angular";

@Injectable({
  providedIn: "root"
})
export class DatabaseService {
  //localUrl = 'http://localhost/api';
  //made some changes for testing
  localUrl = "http://localhost:8000"; //when testing localy with laravel api localy, use this address.
  baseUrl = "http://34.76.28.52/api";
  dispensers: IDispenser[];
  connectedCompany: ICompany;

  constructor(private http: HttpClient) {}

  getDispensers() {
    return this.http.get<IDispenser[]>(
      this.localUrl + "/api/controller/dispenserController.php"
    );
  }

  GetSingleDispensers(id: number) {
    return this.http.get<IDispenser>(this.localUrl + "/api/dispenser/" + id);
  }

  PostDispenser(dispenser: IDispenser, area: IArea) {
    return this.http.post<IDispenser>(`${this.localUrl}/api/dispenser`, {
      id: dispenser.id,
      area_id: area.id
    });
  }

  DeleteDispenser(dispenser: IDispenser) {
    return this.http.delete<IDispenser[]>(
      `${this.localUrl}/api/dispenser/${dispenser.id}`
    );
  }

  GetDispensersOfArea(area: IArea) {
    return this.http.get<IDispenser[]>(
      `${this.localUrl}/api/dispenser?area_id=${area.id}`
    );
  }

  UpdateDispenser(dispenser: IDispenser, area: IArea) {
    return this.http.put<IDispenser>(
      `${this.localUrl}/api/dispenser/${dispenser.id}`,
      {
        area_id: area.id,
        waterlevel: dispenser.waterlevel,
        lastChangedTime: dispenser.lastChangedTime,
        temperature: dispenser.temperature,
        mode: dispenser.mode
      }
    );
  }

  GetArea() {
    return this.http.get<IArea>(this.localUrl + "/api/area");
  }

  GetSingleArea(id: number) {
    return this.http.get<IArea>(this.localUrl + "/api/area/" + id);
  }

  GetAreasOfFloor(floor: IFloor) {
    return this.http.get<IArea[]>(
      `${this.localUrl}/api/area?floor_id=${floor.id}`
    );
  }

  UpdateArea(area: IArea, floor: IFloor) {
    return this.http.put<IArea>(`${this.localUrl}/api/area/${area.id}`, {
      name: area.name,
      conversionRate: area.conversionRate,
      startX: area.startX,
      startY: area.startY,
      endX: area.endX,
      endY: area.endY,
      floor_id: floor.id
    });
  }

  PostArea(area: IArea, floor: IFloor) {
    return this.http.post<IArea>(`${this.localUrl}/api/area`, {
      name: area.name,
      conversionRate: area.conversionRate,
      startX: area.startX,
      startY: area.startY,
      endX: area.endX,
      endY: area.endY,
      floor_id: floor.id
    });
  }

  DeleteArea(area: IArea) {
    return this.http.delete<IArea[]>(`${this.localUrl}/api/area/${area.id}`);
  }

  GetFloor() {
    return this.http.get<IFloor>(this.localUrl + "/api/floor");
  }

  GetSingleFloor(id: number) {
    return this.http.get<IFloor>(this.localUrl + "/api/floor/" + id);
  }

  UpdateFloor(floor: IFloor, building: IBuilding) {
    return this.http.put<IFloor>(`${this.localUrl}/api/floor/${floor.id}`, {
      name: floor.name,
      blueprint: floor.blueprint,
      building_id: building.id
    });
  }

  PostFloor(floor: IFloor, building: IBuilding) {
    return this.http.post<IFloor>(`${this.localUrl}/api/floor`, {
      name: floor.name,
      blueprint: floor.blueprint,
      building_id: building.id
    });
  }

  DeleteFloor(floor: IFloor) {
    return this.http.delete<IFloor[]>(`${this.localUrl}/api/floor/${floor.id}`);
  }

  GetFloorsOfBuilding(building: IBuilding) {
    return this.http.get<IFloor[]>(
      `${this.localUrl}/api/floor?building_id=${building.id}`
    );
  }

  getBuildings() {
    return this.http.get<IBuilding[]>(`${this.localUrl}/api/building`);
  }

  GetSingleBuilding(id?: number) {
    return this.http.get<IBuilding>(this.localUrl + "/api/building/" + id);
  }

  GetBuildingsOfCompany() {
    return this.http.get<IBuilding[]>(
      `${this.localUrl}/api/building?company_id=${this.connectedCompany.id}`
    );
  }

  UpdateBuilding(building: IBuilding) {
    return this.http.put<IBuilding>(
      `${this.localUrl}/api/building/${building.id}`,
      {
        name: building.name,
        city: building.city,
        ZIPcode: building.ZIPcode,
        address: building.address,
        company_id: this.connectedCompany.id
      }
    );
  }

  PostBuilding(building: IBuilding) {
    return this.http.post<IBuilding>(`${this.localUrl}/api/building`, {
      company_id: this.connectedCompany.id,
      name: building.name,
      city: building.city,
      ZIPcode: building.ZIPcode,
      address: building.address
    });
  }

  DeleteBuilding(building) {
    return this.http.delete<IBuilding[]>(
      `${this.localUrl}/api/building/${building.id}`
    );
  }

  GetCompany(compName?: string) {
    return this.http.get<ICompany>(`${this.localUrl}/api/company`, {
      params: new HttpParams().set("companyName", compName)
    });
  }

  GetSingleCompany(id: number) {
    return this.http.get<ICompany>(this.localUrl + "/api/company/" + id);
  }

  PostCompany(compName: string): Observable<ICompany> {
    return this.http.post<ICompany>(`${this.localUrl}/api/company`, {
      name: compName
    });
  }
}

export enum IMode {
  on,
  off
}

export interface IDispenser {
  id: number;
  area_id: number;
  waterlevel: number;
  temperature: number;
  mode: IMode;
  lastChangedTime: string;
  lastCleanedTime: string;
}

export interface IArea {
  id: number;
  floor_id: number;
  name: string;
  conversionRate: number;
  startX: number;
  startY: number;
  endX: number;
  endY: number;
  created_at: string;
  updated_at: string;
}

export interface IFloor {
  id: number;
  building_id: number;
  name: string;
  blueprint: string;
  created_at: string;
  updated_at: string;
}

export interface IBuilding {
  id: number;
  company_id: number;
  name: string;
  city: string;
  ZIPcode: number;
  address: string;
  created_at: string;
  updated_at: string;
}

export interface ICompany {
  id: number;
  name: string;
  created_at: string;
  updated_at: string;
}
