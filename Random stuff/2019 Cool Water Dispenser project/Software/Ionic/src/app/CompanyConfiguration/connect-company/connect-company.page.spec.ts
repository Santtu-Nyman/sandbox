import { CUSTOM_ELEMENTS_SCHEMA } from '@angular/core';
import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { ConnectCompanyPage } from './connect-company.page';

describe('ConnectCompanyPage', () => {
  let component: ConnectCompanyPage;
  let fixture: ComponentFixture<ConnectCompanyPage>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ ConnectCompanyPage ],
      schemas: [CUSTOM_ELEMENTS_SCHEMA],
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(ConnectCompanyPage);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
