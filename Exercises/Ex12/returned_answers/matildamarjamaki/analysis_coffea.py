
#!/usr/bin/env python

import os
import datetime
import hist
import dask
import uproot
import awkward as ak
import hist.dask as hda
import dask_awkward as dak

from coffea import processor
from coffea.nanoevents import NanoEventsFactory, BaseSchema
from coffea.nanoevents.methods import candidate

class Analysis(processor.ProcessorABC):
    def __init__(self):
        self.histograms = {
            "h_mass": (
                hda.Hist.new
                .Reg(200, 0, 200, name="mass", label="DiMuon Mass [GeV]")
                .Double()
            ),
            "h_pileup": (
                hda.Hist.new
                .Reg(100, 0, 100, name="nTrueInt", label="Pileup proxy (PV_npvs)")
                .Double()
            ),
        }

    def process(self, events):
        # Select trigger-passing events
        events = events[events.HLT_IsoMu24]

        # Build muon objects
        muons = ak.zip(
            {
                "pt": events.Muon_pt,
                "eta": events.Muon_eta,
                "phi": events.Muon_phi,
                "mass": events.Muon_mass,
                "charge": events.Muon_charge,
            },
            with_name="PtEtaPhiMCandidate",
            behavior=candidate.behavior,
        )

        # Require exactly 2 opposite-charge muons
        cut = (ak.num(muons) == 2) & (ak.sum(muons.charge, axis=1) == 0)
        dimuon = muons[cut][:, 0] + muons[cut][:, 1]

        # Fill histograms
        self.histograms["h_mass"].fill(mass=dimuon.mass)
        # self.histograms["h_pileup"].fill(nTrueInt=events.Pileup_nTrueInt)
        self.histograms["h_pileup"].fill(nTrueInt=events.PV_npvs)


        return self.histograms

    def postprocess(self, accumulator):
        pass

def main():
    filename = "/home/matildma/Documents/CompinerHEP2025/Exercises/Ex12/returned_answers/matildamarjamaki/DYJetsToLL.root"
    events = NanoEventsFactory.from_root(
        {filename: "Events"},
        schemaclass=BaseSchema,
        metadata={"dataset": "DYJetsToLL"},
    ).events()

    p = Analysis()
    out = p.process(events)
    (result,) = dask.compute(out)

    # Save to ROOT file
    with uproot.recreate("output.root") as fout:
        now = datetime.datetime.now()
        fout[f"produced_{now.strftime('%Y%m%d_%H%M%S')}"] = ""
        for key in result:
            fout[key] = result[key]

if __name__ == "__main__":
    main()