package io.github.pablocpas.opencmr2

import android.app.Activity
import android.app.Application
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.text.format.Formatter
import android.view.WindowManager
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.Image
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.safeDrawingPadding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Surface
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.setValue
import androidx.compose.runtime.remember
import androidx.compose.runtime.mutableStateOf
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import androidx.lifecycle.viewmodel.compose.viewModel
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/** Starts the game, after importing the game files on the first launch. */
class SetupActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            SetupTheme {
                var installed by remember { mutableStateOf(GameData.isInstalled(this)) }
                val model: SetupViewModel = viewModel()
                val state by model.state.collectAsState()
                // Keep the screen on while copying: the import stops if the app is closed.
                LaunchedEffect(state is SetupState.Working) {
                    if (state is SetupState.Working) window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
                    else window.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
                }
                LaunchedEffect(state) {
                    if (state is SetupState.Done) installed = true
                }
                if (installed) LauncherScreen(
                    onStart = ::startGame,
                    onControls = { startActivity(Intent(this, ControllerActivity::class.java)) },
                ) else SetupScreen(state, onFolderChosen = model::import, onCancel = model::cancel)
            }
        }
    }

    private fun startGame() {
        startActivity(Intent(this, GameActivity::class.java))
        finish()
    }
}

sealed interface SetupState {
    data object Welcome : SetupState
    data class Working(val progress: GameData.Progress) : SetupState
    data class Failed(val error: Throwable) : SetupState
    data object Done : SetupState
}

class SetupViewModel(application: Application) : AndroidViewModel(application) {
    private val _state = MutableStateFlow<SetupState>(SetupState.Welcome)
    val state: StateFlow<SetupState> = _state
    private var job: Job? = null

    fun import(tree: Uri) {
        if (job?.isActive == true) return
        job = viewModelScope.launch {
            _state.value = try {
                withContext(Dispatchers.IO) {
                    GameData.import(getApplication(), tree) { _state.value = SetupState.Working(it) }
                }
                SetupState.Done
            } catch (e: CancellationException) {
                SetupState.Welcome
            } catch (e: Exception) {
                SetupState.Failed(e)
            }
        }
    }

    fun cancel() {
        job?.cancel()
    }
}

@Composable
internal fun SetupTheme(content: @Composable () -> Unit) {
    val accent = Color(0xFF01899D)
    val colors = if (isSystemInDarkTheme()) darkColorScheme(primary = Color(0xFF5CD5EA))
                 else lightColorScheme(primary = accent)
    MaterialTheme(colorScheme = colors) {
        Surface(modifier = Modifier.fillMaxSize(), content = content)
    }
}

@Composable
private fun SetupScreen(state: SetupState, onFolderChosen: (Uri) -> Unit, onCancel: () -> Unit) {
    val chooser = rememberLauncherForActivityResult(ActivityResultContracts.OpenDocumentTree()) { uri ->
        if (uri != null) onFolderChosen(uri)
    }
    Box(
        modifier = Modifier.fillMaxSize().safeDrawingPadding().verticalScroll(rememberScrollState()),
        contentAlignment = Alignment.Center,
    ) {
        Column(
            modifier = Modifier.widthIn(max = 560.dp).fillMaxWidth().padding(24.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.spacedBy(16.dp),
        ) {
            Image(
                painter = painterResource(R.drawable.logo),
                contentDescription = null,
                modifier = Modifier.size(96.dp).clip(RoundedCornerShape(24.dp)),
            )
            Text(stringResource(R.string.setup_title), style = MaterialTheme.typography.headlineMedium)
            when (state) {
                SetupState.Welcome, SetupState.Done -> {
                    Text(stringResource(R.string.setup_intro), textAlign = TextAlign.Center)
                    Text(
                        stringResource(R.string.setup_where),
                        style = MaterialTheme.typography.bodySmall,
                        textAlign = TextAlign.Center,
                    )
                    Button(onClick = { chooser.launch(null) }) { Text(stringResource(R.string.setup_choose)) }
                }
                is SetupState.Working -> Working(state.progress, onCancel)
                is SetupState.Failed -> {
                    Text(
                        errorText(state.error),
                        color = MaterialTheme.colorScheme.error,
                        textAlign = TextAlign.Center,
                    )
                    Button(onClick = { chooser.launch(null) }) { Text(stringResource(R.string.setup_choose_again)) }
                }
            }
        }
    }
}

@Composable
private fun Working(progress: GameData.Progress, onCancel: () -> Unit) {
    val context = LocalContext.current
    when (progress) {
        GameData.Progress.Searching -> {
            Text(stringResource(R.string.setup_searching))
            LinearProgressIndicator(modifier = Modifier.fillMaxWidth())
        }
        is GameData.Progress.Copying -> {
            Text(
                stringResource(
                    R.string.setup_copying,
                    Formatter.formatShortFileSize(context, progress.copied),
                    Formatter.formatShortFileSize(context, progress.total),
                )
            )
            LinearProgressIndicator(
                progress = { if (progress.total > 0) progress.copied.toFloat() / progress.total else 0f },
                modifier = Modifier.fillMaxWidth(),
            )
        }
    }
    TextButton(onClick = onCancel) { Text(stringResource(android.R.string.cancel)) }
}

@Composable
private fun errorText(error: Throwable): String {
    val context = LocalContext.current
    return when (error) {
        is GameData.ImportException.NotFound -> stringResource(R.string.setup_not_found)
        is GameData.ImportException.Ambiguous ->
            stringResource(R.string.setup_ambiguous, error.folders.take(4).joinToString("\n") { "• " + it.ifEmpty { "/" } })
        is GameData.ImportException.NoSpace -> stringResource(
            R.string.setup_no_space,
            Formatter.formatShortFileSize(context, error.needed),
            Formatter.formatShortFileSize(context, error.available),
        )
        else -> stringResource(R.string.setup_failed, error.localizedMessage ?: error.toString())
    }
}

@Composable
private fun LauncherScreen(onStart: () -> Unit, onControls: () -> Unit) {
    Box(Modifier.fillMaxSize().safeDrawingPadding().verticalScroll(rememberScrollState()), contentAlignment = Alignment.Center) {
        Column(Modifier.widthIn(max = 560.dp).fillMaxWidth().padding(24.dp),
            horizontalAlignment = Alignment.CenterHorizontally, verticalArrangement = Arrangement.spacedBy(16.dp)) {
            Image(painterResource(R.drawable.logo), null, Modifier.size(96.dp).clip(RoundedCornerShape(24.dp)))
            Text(stringResource(R.string.app_name), style = MaterialTheme.typography.headlineMedium)
            Button(onClick = onStart, modifier = Modifier.fillMaxWidth()) { Text(stringResource(R.string.launcher_start)) }
            TextButton(onClick = onControls) { Text(stringResource(R.string.controllers_title)) }
            DiagnosticControls()
        }
    }
}

// DIAG issue 46: the test switch and the log sharing of the diagnostic build.
@Composable
private fun DiagnosticControls() {
    val context = LocalContext.current
    var fade by remember { mutableStateOf(DiagnosticLog.distanceFade(context)) }
    var noLog by remember { mutableStateOf(false) }
    Text("Diagnostic build (issue 46)", style = MaterialTheme.typography.titleSmall)
    Row(Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
        Text("Distance fade", Modifier.weight(1f))
        Switch(checked = fade, onCheckedChange = { fade = it; DiagnosticLog.setDistanceFade(context, it) })
    }
    OutlinedButton(onClick = { noLog = !DiagnosticLog.share(context as Activity) }, modifier = Modifier.fillMaxWidth()) {
        Text("Share log")
    }
    if (noLog)
        Text("No log yet: play first.", style = MaterialTheme.typography.bodySmall)
}
