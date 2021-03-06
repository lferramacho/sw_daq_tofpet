{
        gStyle->SetPalette(1);
        TCanvas *c = new TCanvas();
        c->Divide(2, 1);
        c->cd(1);
        TGraph2D *g1 =  new TGraph2D("ctr.txt", "%lg %lg %lg %*lg %*lg %*lg %*lg %*lg %*lg %*lg");   
        g1->SetName("g1");
        g1->Draw("COLZ");

//        c->cd(2);
//        TGraph2D *g2 =  new TGraph2D("str.txt", "%lg %lg %*lg %lg %*lg %*lg %*lg %*lg");
//        g2->SetName("g2");
//        g2->Draw("COLZ");
	
//        c->cd(3);
//        TGraph2D *g3 =  new TGraph2D("str.txt", "%lg %lg %*lg %*lg %*lg %*lg %lg %*lg");
//        g3->SetName("g3");
//        g3->Draw("COLZ");
	
	
        c->cd(2);
        TGraph2D *g4 =  new TGraph2D("ctr.txt", "%lg %lg %*lg %*lg %*lg %*lg %lg %*lg %*lg %*lg ");
        g4->SetName("g4");
        g4->Draw("COLZ");
	
        
        c1->SaveAs("/tmp/tofpet_dummy.pdf");

        c->cd(1);
        g1->SetTitle("ToT average [ns]");
        g1->GetXaxis()->SetTitle("postamp [ADC]");
        g1->GetYaxis()->SetTitle("v_th [ADC]");
        g1->GetZaxis()->SetRangeUser(130, 200);
        g1->Draw("COLZ");
        
//        c->cd(2);
//        g2->SetTitle("E resolution [LSB #sigma]");
//        g2->GetXaxis()->SetTitle("ib1 [ADC]");
//        g2->GetYaxis()->SetTitle("vbl [ADC]");
//        g2->GetZaxis()->SetRangeUser(0, 40);
//        g2->Draw("COLZ");
	
//        c->cd(3);
//        g3->SetTitle("T average [LSB]");
//        g3->GetXaxis()->SetTitle("ib1 [ADC]");
//        g3->GetYaxis()->SetTitle("vbl [ADC]");
//        g3->GetZaxis()->SetRangeUser(0, 1024);
//        g3->Draw("COLZ");
        
        c->cd(2);
        g4->SetTitle("CTR [s] ");
        g4->GetXaxis()->SetTitle("postamp [ADC]");
        g4->GetYaxis()->SetTitle("v_th [ADC]");
	
	// Pick a good range!!
        //g4->GetZaxis()->SetRangeUser(75, 250);	
	g4->GetZaxis()->SetRangeUser(100,600);
	//g4->GetZaxis()->SetRangeUser(0,100);
	g4->GetXaxis()->SetRangeUser(0, 64);
	g4->GetYaxis()->SetRangeUser(0, 64);
	
        g4->Draw("COLZ");
	
	
	c1->SaveAs("graph2d.pdf");
	c1->SaveAs("graph2d.png");

}
