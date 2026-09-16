# SDS_110 – Acoustic Detection System (Patent Stefan_FSL5, FIG. 1)

Modulnummern entsprechen den Bezugszeichen des Patents.
Datenfluss (Claim 1 / Claim 10):

  112 Sensor Unit ─┐
   114 Mic Array   │  synchrone Frames
   116 Sampling    ├──► 122 Feature Extraction ──► 124 ML (s(t) = p_1..p_B)
   118 Pre-Proc   ─┘                                   │
                                                       ▼
  140 ◄── 130 Output Interface ◄── 128 Localisation ◄── 126 Correlation (S(t), w(t,k), GCC-PHAT, TDOA)
   ▲                                                    ▲
   └── Feedback ŝ, x̂(t+1) von Tracking Unit 150 ────────┘

Regel (Claim 10): Kein Modul in SDS_110 bildet eine Trajektorie.
Tracking (150–162) liegt außerhalb dieses Projekts.
