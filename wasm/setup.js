var Module = {
  print: (function () {
    var e = document.getElementById("output");
    if (!e) {
      return;
    }
    return (
      e && (e.value = ""),
      (...t) => {
        var n = t.join(" ");
        console.log(n),
          e && ((e.value += n + "\n"), (e.scrollTop = e.scrollHeight));
      }
    );
  })(),
  canvas: (() => {
    var e = document.getElementById("canvas");
    if (!e) {
      return;
    }
    return (
      e.addEventListener(
        "webglcontextlost",
        (e) => {
          alert("WebGL context lost. You will need to reload the page."),
            e.preventDefault();
        },
        !1
      ),
      e
    );
  })(),
};
