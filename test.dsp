import("stdfaust.lib");

gate = button("gate");
freq = nentry("freq", 440, 20, 20000, 1);
gain = hslider("gain", 0.5, 0, 1, 0.01);

process = os.osc(freq) * gate * gain;
