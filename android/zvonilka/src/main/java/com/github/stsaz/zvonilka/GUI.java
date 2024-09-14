/** zvonilka/Android
2024, Simon Zolin */

package com.github.stsaz.zvonilka;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.widget.Toast;

class GUI {
	private static final String TAG = "zvonilka.GUI";
	private Core core;
	Context cur_activity;

	GUI(Core core) {
		this.core = core;
	}

	void on_error(String fmt, Object... args) {
		if (cur_activity == null)
			return;
		msg_show(cur_activity, fmt, args);
	}

	void msg_show(Context ctx, String fmt, Object... args) {
		Toast.makeText(ctx, String.format(fmt, args), Toast.LENGTH_SHORT).show();
	}

	void dlg_question(Context ctx, String title, String msg, String btn_yes, String btn_no, DialogInterface.OnClickListener on_click) {
		new AlertDialog.Builder(ctx)
			.setTitle(title)
			.setMessage(msg)
			.setPositiveButton(btn_yes, on_click)
			.setNegativeButton(btn_no, null)
			.setIcon(android.R.drawable.ic_dialog_alert)
			.show();
	}
}
