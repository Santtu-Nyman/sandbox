import { TestBed } from '@angular/core/testing';

import { ConnectedBuildingInfoService } from './connected-building-info.service';

describe('ConnectedBuildingInfoService', () => {
  beforeEach(() => TestBed.configureTestingModule({}));

  it('should be created', () => {
    const service: ConnectedBuildingInfoService = TestBed.get(ConnectedBuildingInfoService);
    expect(service).toBeTruthy();
  });
});
