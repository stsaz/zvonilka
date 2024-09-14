/** zvonilka/Android
2024, Simon Zolin */

package com.github.stsaz.zvonilka;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import com.github.stsaz.zvonilka.databinding.MainBinding;

public class MainActivity extends AppCompatActivity {
	private static final String TAG = "zvonilka.MainActivity";
	private Core core;
	private MainBinding b;
	private Zvonilka.Ctl ctl;

	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		core = Core.init_once(getApplicationContext());
		if (core == null)
			return;
		core.dbglog(TAG, "onCreate()");

		ctl = new Zvonilka.Ctl() {
			public void open(int flags) {
				core.tq.post(() -> {
					call_on_open(flags);
				});
			}
			public void close(int flags, String msg) {
				core.tq.post(() -> {
					call_on_close(flags, msg);
				});
			}
			public void process(int flags) {
				core.tq.post(() -> {
					call_on_process(flags);
				});
			}
		};

		b = MainBinding.inflate(getLayoutInflater());
		ui_init();
	}

	protected void onStart() {
		super.onStart();
		core.dbglog(TAG, "onStart()");
		ui_show();
	}

	protected void onResume() {
		super.onResume();
		if (core != null)
			core.dbglog(TAG, "onResume()");
	}

	protected void onStop() {
		if (core != null)
			core.dbglog(TAG, "onStop()");
		super.onStop();
	}

	public void onDestroy() {
		if (core != null) {
			core.dbglog(TAG, "onDestroy()");
			core.close();
		}
		super.onDestroy();
	}

	public boolean onCreateOptionsMenu(Menu menu) {
		b.toolbar.inflateMenu(R.menu.menu);
		b.toolbar.setOnMenuItemClickListener(this::onOptionsItemSelected);
		return true;
	}

	public boolean onOptionsItemSelected(@NonNull MenuItem item) {
		switch (item.getItemId()) {
		case R.id.action_settings:
			startActivity(new Intent(this, SettingsActivity.class));
			return true;

		case R.id.action_about:
			startActivity(new Intent(this, AboutActivity.class));
			return true;
		}
		return super.onOptionsItemSelected(item);
	}

	private static final int
		REQUEST_PERM_RECORD = 1;

	/** Called by OS with the result of requestPermissions(). */
	public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
		super.onRequestPermissionsResult(requestCode, permissions, grantResults);
		if (grantResults.length != 0)
			core.dbglog(TAG, "onRequestPermissionsResult: %d: %d", requestCode, grantResults[0]);
	}

	private boolean user_ask_record() {
		String perm = Manifest.permission.RECORD_AUDIO;
		if (ActivityCompat.checkSelfPermission(this, perm) != PackageManager.PERMISSION_GRANTED) {
			core.dbglog(TAG, "ActivityCompat.requestPermissions: %s", perm);
			ActivityCompat.requestPermissions(this, new String[]{perm}, REQUEST_PERM_RECORD);
			return false;
		}
		return true;
	}

	private void ui_init() {
		setContentView(b.getRoot());

		setSupportActionBar(b.toolbar);

		b.eTarget.setText(core.settings.target);
		b.bListen.setOnClickListener((v) -> listen());
		b.bCall.setOnClickListener((v) -> call());
		b.bDisconnect.setOnClickListener((v) -> disconnect());
		b.bDisconnect.setEnabled(false);
		core.gui.cur_activity = this;
	}

	private void ui_show() {
		state_set(core.state);
		if (core.state2 != 0)
			call_on_process(0);
	}

	private void listen() {
		if (core.state != 0)
			return;

		if (!user_ask_record())
			return;

		core.zvon.listen(core.settings.zvon(), this.ctl);
		state_set(1);
	}

	private void call() {
		if (core.state != 0)
			return;

		if (!user_ask_record())
			return;

		String target = b.eTarget.getText().toString();
		if (0 != core.zvon.call(core.settings.zvon(), this.ctl, target)) {
			core.errlog(TAG, "Error");
			return;
		}
		state_set(2);
	}

	private void disconnect() {
		if (core.state == 0)
			return;

		core.zvon.disconnect();
		b.lStatus.setText("");
		state_set(0);
		core.state2 = 0;
	}

	private void state_set(int i) {
		if (i == 0) {
			b.bListen.setEnabled(true);
			b.bCall.setEnabled(true);
			b.bDisconnect.setEnabled(false);
			core.state = 0;

		} else {
			b.bListen.setEnabled(false);
			b.bCall.setEnabled(false);
			b.bDisconnect.setEnabled(true);
			core.state = i;

			if (i == 1) {
				StringBuilder s = new StringBuilder();
				s.append("Listening:\n");
				String[] ips = core.zvon.listIPAddresses();
				for (String ip : ips) {
					s.append(String.format("%s\n", ip));
				}
				b.lStatus.setText(s.toString());

			} else if (i == 2) {
				b.lStatus.setText("Calling...");
			}
		}
	}

	private void call_on_open(int flags) {
		startService(new Intent(this, RecSvc.class));
		if (0 != (flags & 1))
			b.lStatus.setText("Incoming call");
	}

	private void call_on_close(int flags, String msg) {
		String s = "The call is finished";
		if (0 != (flags & 1))
			s = String.format("Error: %s", msg);
		if (0 != (flags & 2))
			s = "The call was interrupted";
		b.lStatus.setText(s);
		disconnect();
		stopService(new Intent(this, RecSvc.class));
	}

	private void call_on_process(int flags) {
		b.lStatus.setText("Speak");
		core.state2 = 1;
	}
}
