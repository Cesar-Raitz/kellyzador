//▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬
// hmsTime.h - A simple structure to keep a timer and automatically generate
//    XXhYYm and ZZs text upon each second increment to show on a LCD display.
//
// Author: Cesar Raitz
// Date: Sep. 2, 2024
//▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬
struct hmsTime {
	byte h, m, s;
	char hm_str[8];
	char s_str[8];
	bool print_hm;

	hmsTime() {reset();}

	void reset() {
		h = m = s = 0;
		gen_hm();
		gen_s();
	}
	void inc() {
		if (++s == 60) {
			s = 0;
			if (++m == 60) {
				m = 0;
				h++;
			}
			gen_hm();
		}
		gen_s();
	}
	void gen_hm() {
		sprintf(hm_str, "%3dh%02dm", (int)h, (int)m);
		print_hm = true;
	}
	void gen_s() { sprintf(s_str, "%02ds", (int)s); }
};