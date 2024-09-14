/** zvonilka/Android
2024, Simon Zolin */

package com.github.stsaz.zvonilka;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;

import com.github.stsaz.zvonilka.databinding.AboutBinding;

public class AboutActivity extends AppCompatActivity {
	private static final String TAG = "zvonilka.AboutActivity";
	Core core;
	private AboutBinding b;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		b = AboutBinding.inflate(getLayoutInflater());
		setContentView(b.getRoot());

		core = Core.ref();

		b.lAbout.setText(String.format("v%s\n\n%s",
			core.zvon.version(),
			"https://github.com/stsaz/zvonilka"));
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
	}
}
