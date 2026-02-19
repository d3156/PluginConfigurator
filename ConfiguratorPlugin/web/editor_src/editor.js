import {EditorView, basicSetup} from "codemirror";
import {EditorState} from "@codemirror/state";
import {json} from "@codemirror/lang-json";

export function createJsonEditor({parent, doc, onChange}) {
  const state = EditorState.create({
    doc: doc ?? "",
    extensions: [
      basicSetup,
      json(),
      EditorView.updateListener.of((upd) => {
        if (upd.docChanged && onChange) onChange(upd.state.doc.toString());
      })
    ]
  });

  const view = new EditorView({state, parent});
  return {
    view,
    getValue: () => view.state.doc.toString(),
    setValue: (text) => view.dispatch({changes: {from: 0, to: view.state.doc.length, insert: text}})
  };
}
