import { Injectable } from '@angular/core';
import { DatabaseService, ICompany } from './database.service';

@Injectable({
  providedIn: 'root'
})
export class ConnectedBuildingInfoService {

  company: ICompany;

  constructor(private dbService: DatabaseService) { 

  }
}
