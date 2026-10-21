type t('ok, 'err) = (result('ok, 'err) => unit) => unit;

let succeed = (value, done_) => done_(Ok(value));
let fail = (error, done_) => done_(Error(error));
let map = (f, task, done_) => task(result => done_(Result.map(f, result)));
let map_error = (f, task, done_) =>
  task(result => done_(Result.map_error(f, result)));
let and_then = (f, task, done_) =>
  task(result =>
    switch (result) {
    | Ok(value) => f(value, done_)
    | Error(error) => done_(Error(error))
    }
  );
let on_error = (f, task, done_) =>
  task(result =>
    switch (result) {
    | Ok(value) => done_(Ok(value))
    | Error(error) => f(error, done_)
    }
  );
let sequence = (tasks, done_) => {
  let rec loop = (values, tasks) =>
    switch (tasks) {
    | [] => done_(Ok(List.rev(values)))
    | [task, ...rest] =>
      task(result =>
        switch (result) {
        | Ok(value) => loop([value, ...values], rest)
        | Error(error) => done_(Error(error))
        }
      )
    };
  loop([], tasks);
};
let perform = (tagger, task) =>
  Cmd.effect(dispatch =>
    task(result =>
      switch (result) {
      | Ok(value) => dispatch(tagger(value))
      | Error () => ()
      }
    )
  );
let attempt = (tagger, task) =>
  Cmd.effect(dispatch => task(result => dispatch(tagger(result))));
